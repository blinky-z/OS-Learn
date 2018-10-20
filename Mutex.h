#ifndef THREADSLEARN_MUTEX_H
#define THREADSLEARN_MUTEX_H


#include <atomic>
#include <thread>

class Mutex {
private:
    std::atomic_flag lock = ATOMIC_FLAG_INIT;
public:
    void acquire();

    void release();
};


#endif //THREADSLEARN_MUTEX_H
