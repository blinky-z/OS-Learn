#ifndef THREADSLEARN_MUTEX_H
#define THREADSLEARN_MUTEX_H

#include <sched.h>

class Mutex {
private:
    bool lock;
public:
    void acquire();

    void release();

    bool try_lock();

    Mutex() {
        lock = false;
    }
};


#endif //THREADSLEARN_MUTEX_H
