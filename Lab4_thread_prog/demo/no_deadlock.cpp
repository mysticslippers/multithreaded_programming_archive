#include <pthread.h>
#include <unistd.h>

#include <iostream>
#include <thread>

using namespace std;

constexpr useconds_t LOCK_DELAY_US = 5000;

pthread_mutex_t first_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t second_mutex = PTHREAD_MUTEX_INITIALIZER;

void worker(int id) {
    // оба потока захватывают мьютексы в одном порядке: first -> second.
    pthread_mutex_lock(&first_mutex);
    usleep(LOCK_DELAY_US);

    pthread_mutex_lock(&second_mutex);
    cout << "worker " << id << ": locked first -> second\n";
    usleep(LOCK_DELAY_US);

    pthread_mutex_unlock(&second_mutex);
    pthread_mutex_unlock(&first_mutex);
}

int main() {
    thread first_worker(worker, 1);
    thread second_worker(worker, 2);

    first_worker.join();
    second_worker.join();

    cout << "no_deadlock finished\n";
    return 0;
}