#include "Spinlock.h"

void Spinlock::acquire() {
    while (!this->try_lock()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

void Spinlock::release() {
    __atomic_clear(&lock, 0);
}

bool Spinlock::try_lock() {
    if (__atomic_test_and_set(&lock, 0)) { // prev value was 1 (lock is already acquired)
        return false;
    } else {
        return true; // successful
    }
}
