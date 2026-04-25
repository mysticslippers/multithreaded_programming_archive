#include <pthread.h>
#include <unistd.h>

#include <atomic>
#include <iostream>
#include <thread>

pthread_mutex_t a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t b = PTHREAD_MUTEX_INITIALIZER;

std::atomic<bool> first_done{false};

void thread1() {
    pthread_mutex_lock(&a);
    usleep(5000);

    pthread_mutex_lock(&b);
    std::cout << "thread1: locked a -> b\n";
    usleep(5000);

    pthread_mutex_unlock(&b);
    pthread_mutex_unlock(&a);

    first_done.store(true, std::memory_order_release);
}

void thread2() {
    while (!first_done.load(std::memory_order_acquire)) {
        usleep(1000);
    }

    pthread_mutex_lock(&b);
    usleep(5000);

    int rc = pthread_mutex_trylock(&a);
    if (rc == 0) {
        std::cout << "thread2: locked b -> a\n";
        pthread_mutex_unlock(&a);
    } else {
        std::cout << "thread2: trylock(a) failed unexpectedly with code " << rc << "\n";
    }

    pthread_mutex_unlock(&b);
}

int main() {
    std::thread t1(thread1);
    std::thread t2(thread2);

    t1.join();
    t2.join();

    std::cout << "deadlock_abba finished\n";
    return 0;
}