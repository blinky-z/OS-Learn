#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include <iostream>
#include <vector>
#include <atomic>
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
        cnt = 0;
    }
}

void try_acquire(bool* can_lock) {
    *can_lock = lock.try_lock();
}

TEST_CASE("Test Mutual exclusion") {
    THREADS_NUM = 2;

    for (int current_try = 0; current_try < 10; current_try++) {
        bool can_lock[THREADS_NUM];
        vector<thread> threads;
        threads.reserve(THREADS_NUM);
        for (int i = 0; i < THREADS_NUM; i++) {
            threads.emplace_back(thread(try_acquire, &can_lock[i]));
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

void do_acquire(atomic<bool>* can_lock) {
    while (!(*can_lock = lock.try_lock()));
}

void find_n_prime_number(int n, atomic<bool>* can_lock) {
    lock.acquire();
    *can_lock = true;

    int current_prime_number_cnt = 0;

    for (int i = 2; i > 0; i++) {
        current_prime_number_cnt++;
        int upper_bound = static_cast<int>(sqrt(i));
        for (int j = 2; j <= upper_bound; j++) {
            if (i % j == 0) {
                current_prime_number_cnt--;
                break;
            }
        }

        if (current_prime_number_cnt == n) { // found n-th prime number
            break;
        }
    }

    lock.release();
    *can_lock = false;
}

TEST_CASE("Test Mutual Exclusion 2") {
    THREADS_NUM = 2;

    for (int current_try = 0; current_try < 10; current_try++) {
        atomic<bool> can_lock[THREADS_NUM];
        vector<thread> threads;
        threads.reserve(THREADS_NUM);

        threads.emplace_back(thread(find_n_prime_number, 100, &can_lock[0]));
        threads.emplace_back(thread(do_acquire, &can_lock[1]));

        for (int i = 0; i < THREADS_NUM; i++) {
            threads[i].join();
        }
        lock.release();

        while (can_lock[0]) {
            REQUIRE(!can_lock[1]); // thread 1 can not acquire a lock while thread 0 already acquired it
        }
        REQUIRE(can_lock[1]); // thread 1 can acquire a lock if thread 0 released it
    }
}