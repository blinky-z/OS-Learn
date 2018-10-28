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

int THREADS_NUM;
int OP_PER_THREAD;

void do_increment(int thread_num, int* cnt) {
    for (int i = 0; i < OP_PER_THREAD; i++) {
        lock.acquire();
        (*cnt)++;
        cout << "[Thread " << thread_num << "] " << *cnt << endl;
        lock.release();
    }
}

TEST_CASE("Concurrent count increment") {
    THREADS_NUM = 100;
    OP_PER_THREAD = 100;

    int cnt = 0;

    for (int current_try = 0; current_try < 10; current_try++) {
        vector<thread> threads;
        threads.reserve(THREADS_NUM);
        for (int current_thread = 0; current_thread < THREADS_NUM; current_thread++) {
            threads.emplace_back(thread(do_increment, current_thread, &cnt));
        }

        for (int i = 0; i < THREADS_NUM; i++) {
            threads[i].join();
        }

        REQUIRE(cnt == THREADS_NUM * OP_PER_THREAD);
        cnt = 0;
    }
}

void do_acquire(volatile atomic<bool>* locked) {
    *locked = lock.try_lock();
    while (!(*locked)) {
        cout << "Can not acquire\n";
        *locked = lock.try_lock();
    }
}

void find_n_prime_number(int n, volatile atomic<bool>* locked) {
    lock.acquire();
    *locked = true;

    int prime_numbers_cnt = 0;
    int current_num = 2;

    while (true) {
        prime_numbers_cnt++;
        for (int j = 2; j <= static_cast<int>(sqrt(current_num)); j++) {
            if (current_num % j == 0) {
                prime_numbers_cnt--;
                break;
            }
        }

        if (prime_numbers_cnt == n) { // found n-th prime number
            break;
        }
    }

    lock.release();
    *locked = false;
}

TEST_CASE("Unable acquire a lock while other thread already acquired it") {
    THREADS_NUM = 2;

    for (int current_try = 0; current_try < 10; current_try++) {
        volatile atomic<bool> locked[THREADS_NUM]; // monitoring which thread acquired a lock

        vector<thread> threads;
        threads.reserve(THREADS_NUM);
        threads.emplace_back(thread(find_n_prime_number, 20, &locked[0]));
        while (!locked[0]); // wait until first thread acquire a lock to put other thread to work
        threads.emplace_back(thread(do_acquire, &locked[1]));

        for (int i = 0; i < THREADS_NUM; i++) {
            threads[i].detach();
        }

        while (locked[0]) {
            REQUIRE(!locked[1]); // thread 1 can not acquire a lock while thread 0 already acquired it
        }
        REQUIRE(locked[1]); // thread 1 can acquire a lock since thread 0 released it

        lock.release();
    }
}