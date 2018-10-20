#include "Mutex.h"

void Mutex::acquire() {
    while (lock.test_and_set()) {
        std::this_thread::yield();
    }
}

void Mutex::release() {
    lock.clear();
}