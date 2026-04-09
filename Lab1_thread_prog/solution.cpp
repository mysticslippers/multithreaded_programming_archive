using namespace std;

class DiningPhilosophers {
private:
    mutex forks[5];
    counting_semaphore<4> possible_seats{4};

public:
    DiningPhilosophers() {}

    void wantsToEat(int philosopher,
                    function<void()> pickLeftFork,
                    function<void()> pickRightFork,
                    function<void()> eat,
                    function<void()> putLeftFork,
                    function<void()> putRightFork) {
        int left_fork_index = philosopher;
        int right_fork_index = (philosopher + 1) % 5;

        possible_seats.acquire();

        forks[left_fork_index].lock();
        forks[right_fork_index].lock();

        pickLeftFork();
        pickRightFork();

        eat();
        
        putLeftFork();
        putRightFork();

        forks[left_fork_index].unlock();
        forks[right_fork_index].unlock();

        possible_seats.release();
    }
};
