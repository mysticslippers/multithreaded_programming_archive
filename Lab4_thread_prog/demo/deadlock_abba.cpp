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

// нужен, чтобы второй поток начал работу после построения ребра a -> b
atomic<bool> first_thread_done{false};

void wait_for_first_thread() {
    while (!first_thread_done.load(memory_order_acquire)) {
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

    first_thread_done.store(true, memory_order_release);
}

void second_thread() {
    wait_for_first_thread();

    pthread_mutex_lock(&mutex_b);
    usleep(LOCK_DELAY_US);

    // trylock нужен, чтобы показать ребро b -> a, но не зависнуть реально в deadlock'е
    const int result = pthread_mutex_trylock(&mutex_a);
    if (result == 0) {
        cout << "thread2: locked b -> a\n";
        pthread_mutex_unlock(&mutex_a);
    } else {
        cout << "thread2: trylock(a) failed unexpectedly with code "
             << result << "\n";
    }

    pthread_mutex_unlock(&mutex_b);
}

int main() {
    thread t1(first_thread);
    thread t2(second_thread);

    t1.join();
    t2.join();

    cout << "deadlock_abba finished\n";
    return 0;
}