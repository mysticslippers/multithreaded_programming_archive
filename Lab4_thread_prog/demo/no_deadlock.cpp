#include <pthread.h>
#include <unistd.h>

#include <iostream>
#include <thread>

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void worker(int id) {
    pthread_mutex_lock(&m1);
    usleep(5000);

    pthread_mutex_lock(&m2);
    std::cout << "worker " << id << ": locked m1 -> m2\n";
    usleep(5000);

    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);

    t1.join();
    t2.join();

    std::cout << "no_deadlock finished\n";
    return 0;
}