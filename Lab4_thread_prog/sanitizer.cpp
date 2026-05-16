#include "sanitizer.h"

#include <dlfcn.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace sanitizer {

namespace {

using LockFn = int (*)(pthread_mutex_t *);
using UnlockFn = int (*)(pthread_mutex_t *);

// указатели на настоящие pthread-функции
LockFn real_pthread_mutex_lock = nullptr;
LockFn real_pthread_mutex_trylock = nullptr;
UnlockFn real_pthread_mutex_unlock = nullptr;

// общее состояние sanitizer'а
std::mutex global_state_mutex;
LockGraph global_graph;

// мьютексы, захваченные текущим потоком
thread_local vector<MutexId> held_mutexes;

// защита от повторного входа в sanitizer
thread_local bool reentry_guard = false;

pid_t get_thread_id() { return static_cast<pid_t>(syscall(SYS_gettid)); }

// получает настоящие pthread-функции через dlsym
void init_real_functions() {
  static once_flag flag;
  call_once(flag, []() {
    real_pthread_mutex_lock =
        reinterpret_cast<LockFn>(dlsym(RTLD_NEXT, "pthread_mutex_lock"));
    real_pthread_mutex_trylock =
        reinterpret_cast<LockFn>(dlsym(RTLD_NEXT, "pthread_mutex_trylock"));
    real_pthread_mutex_unlock =
        reinterpret_cast<UnlockFn>(dlsym(RTLD_NEXT, "pthread_mutex_unlock"));

    if (real_pthread_mutex_lock == nullptr ||
        real_pthread_mutex_trylock == nullptr ||
        real_pthread_mutex_unlock == nullptr) {
      fprintf(stderr, "sanitizer: failed to resolve pthread symbols\n");
      abort();
    }
  });
}

// выводит информацию о найденном потенциальном deadlock'е
void print_deadlock_report(const DeadlockReport &report,
                           MutexId requested_mutex) {
  fprintf(stderr, "\n > sanitizer: potential deadlock detected \n");
  fprintf(stderr, "> thread id: %d\n", static_cast<int>(get_thread_id()));
  fprintf(stderr, "> new edge: 0x%zx -> 0x%zx\n",
          static_cast<size_t>(report.new_from),
          static_cast<size_t>(report.new_to));
  fprintf(stderr, "> requested mutex: 0x%zx\n",
          static_cast<size_t>(requested_mutex));

  fprintf(stderr, "> held mutexes by current thread:");
  if (held_mutexes.empty()) {
    fprintf(stderr, "> <none>\n");
  } else {
    fprintf(stderr, "\n");

    unordered_set<MutexId> printed;
    for (const MutexId mutex : held_mutexes) {
      if (printed.insert(mutex).second) {
        fprintf(stderr, "  - 0x%zx\n", static_cast<size_t>(mutex));
      }
    }
  }

  const string cycle_text = format_cycle(report.cycle);
  fprintf(stderr, "> cycle: %s\n", cycle_text.c_str());
  fprintf(stderr, "\n\n");
}

// обрабатывает успешный захват мьютекса
void on_successful_lock(pthread_mutex_t *pthread_mutex) {
  const MutexId new_mutex = mutex_to_id(pthread_mutex);

  if (reentry_guard) {
    held_mutexes.push_back(new_mutex);
    return;
  }

  reentry_guard = true;
  {
    std::lock_guard<std::mutex> lock(global_state_mutex);

    // добавляем ребра от уже удерживаемых мьютексов к новому
    for (const MutexId already_held : held_mutexes) {
      if (already_held == new_mutex) {
        continue;
      }

      DeadlockReport report = global_graph.add_edge(already_held, new_mutex);
      if (report.has_cycle) {
        print_deadlock_report(report, new_mutex);
        _exit(1);
      }
    }

    held_mutexes.push_back(new_mutex);
  }
  reentry_guard = false;
}

// обрабатывает освобождение мьютекса
void on_unlock(pthread_mutex_t *pthread_mutex) {
  const MutexId target = mutex_to_id(pthread_mutex);

  if (reentry_guard) {
    auto it = find(held_mutexes.rbegin(), held_mutexes.rend(), target);
    if (it != held_mutexes.rend()) {
      held_mutexes.erase(next(it).base());
    }
    return;
  }

  reentry_guard = true;
  {
    std::lock_guard<std::mutex> lock(global_state_mutex);

    auto it = find(held_mutexes.rbegin(), held_mutexes.rend(), target);
    if (it != held_mutexes.rend()) {
      held_mutexes.erase(next(it).base());
    }
  }
  reentry_guard = false;
}

} // namespace

MutexId mutex_to_id(const pthread_mutex_t *mutex) {
  return reinterpret_cast<MutexId>(mutex);
}

// формирует строку с циклом мьютексов
string format_cycle(const MutexCycle &cycle) {
  if (cycle.empty()) {
    return "> <empty>";
  }

  ostringstream out;
  for (size_t i = 0; i < cycle.size(); ++i) {
    out << "0x" << hex << cycle[i];
    if (i + 1 < cycle.size()) {
      out << " -> ";
    }
  }
  return out.str();
}

// ищет путь current -> target в графе
bool LockGraph::find_path(MutexId current, MutexId target, MutexSet &visited,
                          MutexCycle &path) const {
  if (visited.count(current) != 0U) {
    return false;
  }

  visited.insert(current);
  path.push_back(current);

  if (current == target) {
    return true;
  }

  auto it = edges_.find(current);
  if (it != edges_.end()) {
    for (const MutexId next_mutex : it->second) {
      if (find_path(next_mutex, target, visited, path)) {
        return true;
      }
    }
  }

  path.pop_back();
  return false;
}

// добавляет ребро и проверяет появился ли цикл
DeadlockReport LockGraph::add_edge(MutexId from, MutexId to) {
  DeadlockReport report;
  report.new_from = from;
  report.new_to = to;

  if (from == to) {
    return report;
  }

  if (has_edge(from, to)) {
    return report;
  }

  MutexSet visited;
  MutexCycle reverse_path;

  // если уже есть путь to -> from, то ребро from -> to замыкает цикл
  if (find_path(to, from, visited, reverse_path)) {
    report.has_cycle = true;
    report.cycle.push_back(from);
    report.cycle.insert(report.cycle.end(), reverse_path.begin(),
                        reverse_path.end());
  }

  edges_[from].insert(to);
  edges_.try_emplace(to, MutexSet{});

  return report;
}

bool LockGraph::has_edge(MutexId from, MutexId to) const {
  auto it = edges_.find(from);
  if (it == edges_.end()) {
    return false;
  }

  return it->second.count(to) != 0U;
}

size_t LockGraph::vertex_count() const { return edges_.size(); }

size_t LockGraph::edge_count() const {
  size_t total = 0;

  for (const auto &[_, neighbors] : edges_) {
    total += neighbors.size();
  }

  return total;
}

} // namespace sanitizer

extern "C" int pthread_mutex_lock(pthread_mutex_t *mutex) {
  sanitizer::init_real_functions();

  const int result = sanitizer::real_pthread_mutex_lock(mutex);
  if (result == 0) {
    sanitizer::on_successful_lock(mutex);
  }

  return result;
}

extern "C" int pthread_mutex_trylock(pthread_mutex_t *mutex) {
  sanitizer::init_real_functions();

  const int result = sanitizer::real_pthread_mutex_trylock(mutex);
  if (result == 0) {
    sanitizer::on_successful_lock(mutex);
  }

  return result;
}

extern "C" int pthread_mutex_unlock(pthread_mutex_t *mutex) {
  sanitizer::init_real_functions();
  sanitizer::on_unlock(mutex);

  return sanitizer::real_pthread_mutex_unlock(mutex);
}