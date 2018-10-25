#ifndef THREADSLEARN_SPINLOCK_H
#define THREADSLEARN_SPINLOCK_H
#include <atomic>

class Spinlock {
private:
    std::atomic_flag lock = ATOMIC_FLAG_INIT;
public:
    void acquire();

    void release();

    bool try_lock();
};


#endif //THREADSLEARN_SPINLOCK_H
