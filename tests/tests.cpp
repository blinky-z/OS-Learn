#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <condition_variable>
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

//TEST_CASE("Concurrent count increment") {
//    THREADS_NUM = 100;
//    OP_PER_THREAD = 100;
//
//    int cnt = 0;
//
//    for (int current_try = 0; current_try < 10; current_try++) {
//        vector<thread> threads;
//        threads.reserve(THREADS_NUM);
//        for (int current_thread = 0; current_thread < THREADS_NUM; current_thread++) {
//            threads.emplace_back(thread(do_increment, current_thread, &cnt));
//        }
//
//        for (int i = 0; i < THREADS_NUM; i++) {
//            threads[i].join();
//        }
//
//        REQUIRE(cnt == THREADS_NUM * OP_PER_THREAD);
//        cnt = 0;
//    }
//}

void do_acquire(atomic<bool>* locked, condition_variable* is_second_thread_acquired_lock) {
    *locked = lock.try_lock();
    while (!(*locked)) {
        cout << "Thread 2 can not acquire a lock. Sleeping...\n";
        this_thread::sleep_for(chrono::milliseconds(60));
        cout << "Waked up. Trying to acquire a lock again\n";
        *locked = lock.try_lock();
    }
    cout << "Thread 2 successfully acquired a lock\n";
    is_second_thread_acquired_lock->notify_one();
}

void find_n_prime_number(int n, atomic<bool>* locked, condition_variable* is_first_thread_acquired_lock) {
    lock.acquire();
    is_first_thread_acquired_lock->notify_one();
    cout << "Thread 1 acquired a lock\n";
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
    cout << "Thread 1 released a lock\n";
}

TEST_CASE("Unable acquire a lock while other thread already acquired it") {
    THREADS_NUM = 2;

    for (int current_try = 0; current_try < 10; current_try++) {
        atomic<bool> locked[THREADS_NUM]; // monitoring which thread acquired a lock

        vector<thread> threads;
        threads.reserve(THREADS_NUM);

        mutex first_thread_wait_mutex;
        mutex second_thread_wait_mutex;

        condition_variable is_first_thread_acquired_lock;
        condition_variable is_second_thread_acquired_lock;

        threads.emplace_back(thread(find_n_prime_number, 100, &locked[0], &is_first_thread_acquired_lock));

        unique_lock<mutex> first_thread_wait_lock(first_thread_wait_mutex);
        is_first_thread_acquired_lock.wait(first_thread_wait_lock); // wait until first thread acquired a lock to put second one to work

        threads.emplace_back(thread(do_acquire, &locked[1], &is_second_thread_acquired_lock));

        for (int i = 0; i < THREADS_NUM; i++) {
            threads[i].detach();
        }

//        while (locked[0]) {
//            REQUIRE(!locked[1]); // thread 1 can not acquire a lock while thread 0 already acquired it
//        }

        unique_lock<mutex> second_thread_wait_lock(second_thread_wait_mutex);
        is_second_thread_acquired_lock.wait(second_thread_wait_lock);

        cout << "thread 0 state: " << locked[0] << endl;
        cout << "thread 1 state: " << locked[1] << endl;
        REQUIRE(!locked[0]);
        REQUIRE(locked[1]); // thread 1 can acquire a lock since thread 0 released it

        lock.release();
    }
}