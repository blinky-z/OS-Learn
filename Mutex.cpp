#include "Mutex.h"

void Mutex::acquire() {
    while (!try_lock()) {
        std::this_thread::yield();
    }
}

void Mutex::release() {
    __atomic_clear(&lock, 0);
}

bool Mutex::try_lock() {
    if (__atomic_test_and_set(&lock, 0)) {  // prev value was 1 (lock is already acquired)
        return false;
    } else {
        return true;
    }
}