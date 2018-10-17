#ifndef THREADSLEARN_SPINLOCK_H
#define THREADSLEARN_SPINLOCK_H
#include <atomic>

class Spinlock {
private:
    std::atomic_flag lock = ATOMIC_FLAG_INIT;
public:
    void acquire();

    void release();
};


#endif //THREADSLEARN_SPINLOCK_H
