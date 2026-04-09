#include "producer_consumer.h"

#include <pthread.h>
#include <time.h>

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

using namespace std;

namespace {

struct SharedState {
  pthread_mutex_t mutex;
  pthread_mutex_t output_mutex;
  pthread_cond_t value_ready;
  pthread_cond_t value_processed;

  int current_value;
  bool has_value;
  bool producer_finished;

  int sleep_limit_ms;
  bool debug;

  const vector<int>* input_values;
  size_t next_index;

  vector<pthread_t> consumer_threads;
};

struct ConsumerResult {
  int sum;
};

struct ThreadContext {
  SharedState* state;
  int thread_number;
};

pthread_key_t g_tid_key;
pthread_once_t g_tid_key_once = PTHREAD_ONCE_INIT;

void make_tid_key() { pthread_key_create(&g_tid_key, free); }

void set_tid(int tid) {
  pthread_once(&g_tid_key_once, make_tid_key);

  void* current_ptr = pthread_getspecific(g_tid_key);
  if (current_ptr != nullptr) {
    free(current_ptr);
  }

  int* tid_ptr = static_cast<int*>(malloc(sizeof(int)));
  *tid_ptr = tid;
  pthread_setspecific(g_tid_key, tid_ptr);
}

void sleep_for_milliseconds(int milliseconds) {
  if (milliseconds <= 0) {
    return;
  }

  timespec ts{};
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = static_cast<long>(milliseconds % 1000) * 1000000L;
  nanosleep(&ts, nullptr);
}

}

int get_tid() {
  pthread_once(&g_tid_key_once, make_tid_key);

  void* ptr = pthread_getspecific(g_tid_key);
  if (ptr == nullptr) {
    return 0;
  }

  return *static_cast<int*>(ptr);
}

void* producer_routine(void* arg) {
  auto* context = static_cast<ThreadContext*>(arg);
  SharedState* state = context->state;
  set_tid(context->thread_number);

  while (true) {
    pthread_mutex_lock(&state->mutex);

    if (state->next_index >= state->input_values->size()) {
      state->producer_finished = true;
      pthread_cond_broadcast(&state->value_ready);
      pthread_mutex_unlock(&state->mutex);
      break;
    }

    while (state->has_value) {
      pthread_cond_wait(&state->value_processed, &state->mutex);
    }

    state->current_value = (*state->input_values)[state->next_index];
    state->has_value = true;
    ++state->next_index;

    pthread_cond_broadcast(&state->value_ready);

    while (state->has_value) {
      pthread_cond_wait(&state->value_processed, &state->mutex);
    }

    pthread_mutex_unlock(&state->mutex);
  }

  return nullptr;
}

void* consumer_routine(void* arg) {
  auto* context = static_cast<ThreadContext*>(arg);
  SharedState* state = context->state;
  set_tid(context->thread_number);

  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

  auto* result = new ConsumerResult{0};

  while (true) {
    pthread_mutex_lock(&state->mutex);

    while (!state->has_value && !state->producer_finished) {
      pthread_cond_wait(&state->value_ready, &state->mutex);
    }

    if (!state->has_value && state->producer_finished) {
      pthread_mutex_unlock(&state->mutex);
      break;
    }

    const int value = state->current_value;
    state->has_value = false;

    pthread_cond_signal(&state->value_processed);
    pthread_mutex_unlock(&state->mutex);

    result->sum += value;

    if (state->debug) {
      pthread_mutex_lock(&state->output_mutex);
      cout << "(" << get_tid() << ", " << result->sum << ")" << endl;
      pthread_mutex_unlock(&state->output_mutex);
    }

    if (state->sleep_limit_ms > 0) {
      const int sleep_time = rand() % (state->sleep_limit_ms + 1);
      sleep_for_milliseconds(sleep_time);
    }
  }

  return result;
}

void* consumer_interruptor_routine(void* arg) {
  auto* context = static_cast<ThreadContext*>(arg);
  SharedState* state = context->state;
  set_tid(context->thread_number);

  while (true) {
    pthread_mutex_lock(&state->mutex);
    const bool finished = state->producer_finished;
    pthread_mutex_unlock(&state->mutex);

    if (finished) {
      break;
    }

    if (!state->consumer_threads.empty()) {
      const size_t index =
          static_cast<size_t>(rand()) % state->consumer_threads.size();
      pthread_cancel(state->consumer_threads[index]);
    }
  }

  return nullptr;
}

int run_threads(const vector<int>& values, int consumer_count,
                int sleep_limit_ms, bool debug) {
  set_tid(1);

  if (consumer_count <= 0) {
    return 0;
  }

  if (sleep_limit_ms < 0) {
    sleep_limit_ms = 0;
  }

  srand(static_cast<unsigned int>(time(nullptr)));

  SharedState state{};
  pthread_mutex_init(&state.mutex, nullptr);
  pthread_mutex_init(&state.output_mutex, nullptr);
  pthread_cond_init(&state.value_ready, nullptr);
  pthread_cond_init(&state.value_processed, nullptr);

  state.current_value = 0;
  state.has_value = false;
  state.producer_finished = false;
  state.sleep_limit_ms = sleep_limit_ms;
  state.debug = debug;
  state.input_values = &values;
  state.next_index = 0;
  state.consumer_threads.resize(consumer_count);

  pthread_t producer_thread;
  pthread_t interruptor_thread;

  ThreadContext producer_context{&state, 2};
  ThreadContext interruptor_context{&state, 3};
  vector<ThreadContext> consumer_contexts(consumer_count);
  vector<void*> consumer_returns(consumer_count, nullptr);

  for (int i = 0; i < consumer_count; ++i) {
    consumer_contexts[i] = ThreadContext{&state, 4 + i};
    pthread_create(&state.consumer_threads[i], nullptr, consumer_routine,
                   &consumer_contexts[i]);
  }

  pthread_create(&producer_thread, nullptr, producer_routine,
                 &producer_context);
  pthread_create(&interruptor_thread, nullptr, consumer_interruptor_routine,
                 &interruptor_context);

  pthread_join(producer_thread, nullptr);
  pthread_join(interruptor_thread, nullptr);

  int total_sum = 0;
  for (int i = 0; i < consumer_count; ++i) {
    pthread_join(state.consumer_threads[i], &consumer_returns[i]);
    auto* result = static_cast<ConsumerResult*>(consumer_returns[i]);
    total_sum += result->sum;
    delete result;
  }

  pthread_mutex_destroy(&state.mutex);
  pthread_mutex_destroy(&state.output_mutex);
  pthread_cond_destroy(&state.value_ready);
  pthread_cond_destroy(&state.value_processed);

  return total_sum;
}