#include "Mutex.h"

void Mutex::acquire() {
    bool oldValue;
    __atomic_load(&lock, &oldValue, 0);
    while (__atomic_test_and_set(&lock, 0)) {
        sched_yield();
    }
}

void Mutex::release() {
    __atomic_clear(&lock, 0);
}

bool Mutex::try_lock() {
    bool oldValue;
    __atomic_load(&lock, &oldValue, 0);
    if (__atomic_test_and_set(&lock, 0)) {
        return false;
    } else {
        return true;
    }
}