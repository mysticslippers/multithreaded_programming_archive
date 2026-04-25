#include <pthread.h>
#include <unistd.h>

#include <atomic>
#include <iostream>
#include <thread>

pthread_mutex_t a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t b = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t c = PTHREAD_MUTEX_INITIALIZER;

std::atomic<int> stage{0};

void thread1() {
    pthread_mutex_lock(&a);
    usleep(5000);

    pthread_mutex_lock(&b);
    std::cout << "thread1: locked a -> b\n";
    usleep(5000);

    pthread_mutex_unlock(&b);
    pthread_mutex_unlock(&a);

    stage.store(1, std::memory_order_release);
}

void thread2() {
    while (stage.load(std::memory_order_acquire) < 1) {
        usleep(1000);
    }

    pthread_mutex_lock(&b);
    usleep(5000);

    pthread_mutex_lock(&c);
    std::cout << "thread2: locked b -> c\n";
    usleep(5000);

    pthread_mutex_unlock(&c);
    pthread_mutex_unlock(&b);

    stage.store(2, std::memory_order_release);
}

void thread3() {
    while (stage.load(std::memory_order_acquire) < 2) {
        usleep(1000);
    }

    pthread_mutex_lock(&c);
    usleep(5000);

    int rc = pthread_mutex_trylock(&a);
    if (rc == 0) {
        std::cout << "thread3: locked c -> a\n";
        pthread_mutex_unlock(&a);
    } else {
        std::cout << "thread3: trylock(a) failed unexpectedly with code " << rc << "\n";
    }

    pthread_mutex_unlock(&c);
}

int main() {
    std::thread t1(thread1);
    std::thread t2(thread2);
    std::thread t3(thread3);

    t1.join();
    t2.join();
    t3.join();

    std::cout << "deadlock_cycle3 finished\n";
    return 0;
}