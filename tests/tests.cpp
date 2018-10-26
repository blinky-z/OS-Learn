#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include "../Mutex.h"
#include "../Spinlock.h"

using namespace std;

#if TEST_MUTEX
Mutex lock;
#else
Spinlock lock;
#endif

int cnt = 0;

int THREADS_NUM;
int OP_PER_THREAD;

void reset_count() {
    cnt = 0;
}

void do_increment(int thread_num) {
    for (int i = 0; i < OP_PER_THREAD; i++) {
        lock.acquire();
        cnt++;
        cout << "[Thread " << thread_num << "] " << cnt << endl;
        lock.release();
    }
}

TEST_CASE("Concurrent count increment") {
    THREADS_NUM = 100;
    OP_PER_THREAD = 100;

    for (int current_try = 0; current_try < 10; current_try++) {
        vector<thread> threads;
        threads.reserve(THREADS_NUM);
        for (int i = 0; i < THREADS_NUM; i++) {
            threads.emplace_back(thread(do_increment, i));
        }

        for (int i = 0; i < THREADS_NUM; i++) {
            threads[i].join();
        }

        REQUIRE(cnt == THREADS_NUM * OP_PER_THREAD);
        reset_count();
    }
}

void do_acquire(bool* can_lock) {
    *can_lock = lock.try_lock();
}

TEST_CASE("Test Mutual exclusion") {
    THREADS_NUM = 2;

    for (int current_try = 0; current_try < 10; current_try++) {
        bool can_lock[THREADS_NUM];
        vector<thread> threads;
        threads.reserve(THREADS_NUM);
        for (int i = 0; i < THREADS_NUM; i++) {
            threads.emplace_back(thread(do_acquire, &can_lock[i]));
        }

        for (int i = 0; i < THREADS_NUM; i++) {
            threads[i].join();
        }
        lock.release();

        if (can_lock[0]) {
            REQUIRE(!can_lock[1]); // if first thread acquired a lock so other thread can not acquire lock
        } else {
            REQUIRE(can_lock[1]); // if first threads can not acquire a lock so the other thread acquired it
        }
    }
}