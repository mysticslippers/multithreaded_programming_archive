#pragma once

#include <pthread.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

namespace sanitizer {

// указатель на мьютекс
using MutexId = uintptr_t;

// последовательность мьютексов
using MutexCycle = vector<MutexId>;

// сет мьютексов для DFS и хранения соседей
using MutexSet = unordered_set<MutexId>;

// граф захвата мьютексов
using LockEdges = unordered_map<MutexId, MutexSet>;

// результат проверки после добавления нового ребра
struct DeadlockReport {
  bool has_cycle = false;
  MutexCycle cycle;
  MutexId new_from = 0;
  MutexId new_to = 0;
};

// граф, где вершины - мьютексы, а ребра - порядок их захвата
class LockGraph {
public:
  DeadlockReport add_edge(MutexId from, MutexId to);

  [[nodiscard]] bool has_edge(MutexId from, MutexId to) const;

  [[nodiscard]] size_t vertex_count() const;
  [[nodiscard]] size_t edge_count() const;

private:
  LockEdges edges_;

  // ищет путь current -> target в графе
  [[nodiscard]] bool find_path(MutexId current, MutexId target,
                               MutexSet &visited, MutexCycle &path) const;
};

// преобразует pthread_mutex_t* в идентификатор вершины графа
[[nodiscard]] MutexId mutex_to_id(const pthread_mutex_t *mutex);

// формирует строку с найденным циклом для вывода
string format_cycle(const MutexCycle &cycle);

} // namespace sanitizer