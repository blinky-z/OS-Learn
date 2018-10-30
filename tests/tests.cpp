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

condition_variable is_first_thread_acquired_lock;
condition_variable second_thread_can_acquire;

void do_acquire() {
    cout << "thread 2 trying to acquire a lock\n";
    bool locked = lock.try_lock();
    REQUIRE(!locked);

    mutex wait_mutex;
    unique_lock<mutex> wait_lock(wait_mutex);

    second_thread_can_acquire.wait(wait_lock);

    locked = lock.try_lock();
    REQUIRE(locked);
}

void find_n_prime_number(int n) {
    lock.acquire();
    is_first_thread_acquired_lock.notify_one();
    cout << "Thread 1 acquired a lock\n";

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
    cout << "Thread 1 released a lock\n";
    second_thread_can_acquire.notify_one();
}

TEST_CASE("Unable acquire a lock while other thread already acquired it") {
    THREADS_NUM = 2;

    vector<thread> threads;
    threads.reserve(THREADS_NUM);

    mutex first_thread_wait_mutex;
    unique_lock<mutex> first_thread_wait_lock(first_thread_wait_mutex);

    threads.emplace_back(thread(find_n_prime_number, 100)); // put first thread to work (long work)
    is_first_thread_acquired_lock.wait(
            first_thread_wait_lock); // wait until first thread acquired a lock to put second one to work
    threads.emplace_back(
            thread(do_acquire)); // put second thread to work

    for (int i = 0; i < THREADS_NUM; i++) {
        threads[i].join();
    }

    lock.release();
}