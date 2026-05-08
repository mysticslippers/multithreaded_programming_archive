#include <pthread.h>
#include <unistd.h>

#include <atomic>
#include <iostream>
#include <thread>

using namespace std;

constexpr useconds_t LOCK_DELAY_US = 5000;
constexpr useconds_t WAIT_DELAY_US = 1000;

pthread_mutex_t mutex_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_b = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_c = PTHREAD_MUTEX_INITIALIZER;

// нужен, чтобы потоки последовательно построили цикл a -> b -> c -> a
atomic<int> stage{0};

void wait_for_stage(int expected_stage) {
    while (stage.load(memory_order_acquire) < expected_stage) {
        usleep(WAIT_DELAY_US);
    }
}

void first_thread() {
    pthread_mutex_lock(&mutex_a);
    usleep(LOCK_DELAY_US);

    pthread_mutex_lock(&mutex_b);
    cout << "thread1: locked a -> b\n";
    usleep(LOCK_DELAY_US);

    pthread_mutex_unlock(&mutex_b);
    pthread_mutex_unlock(&mutex_a);

    stage.store(1, memory_order_release);
}

void second_thread() {
    wait_for_stage(1);

    pthread_mutex_lock(&mutex_b);
    usleep(LOCK_DELAY_US);

    pthread_mutex_lock(&mutex_c);
    cout << "thread2: locked b -> c\n";
    usleep(LOCK_DELAY_US);

    pthread_mutex_unlock(&mutex_c);
    pthread_mutex_unlock(&mutex_b);

    stage.store(2, memory_order_release);
}

void third_thread() {
    wait_for_stage(2);

    pthread_mutex_lock(&mutex_c);
    usleep(LOCK_DELAY_US);

    // trylock нужен, чтобы показать ребро c -> a, но не зависнуть реально в deadlock'е
    const int result = pthread_mutex_trylock(&mutex_a);
    if (result == 0) {
        cout << "thread3: locked c -> a\n";
        pthread_mutex_unlock(&mutex_a);
    } else {
        cout << "thread3: trylock(a) failed unexpectedly with code "
             << result << "\n";
    }

    pthread_mutex_unlock(&mutex_c);
}

int main() {
    thread t1(first_thread);
    thread t2(second_thread);
    thread t3(third_thread);

    t1.join();
    t2.join();
    t3.join();

    cout << "deadlock_cycle3 finished\n";
    return 0;
}