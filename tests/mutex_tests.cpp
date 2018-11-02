#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <condition_variable>
#include "../Mutex.h"

using namespace std;

Mutex lock;

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

mutex first_thread_acquired_mutex;
condition_variable is_first_thread_acquired_cond;
bool first_thread_acquired_pred = false;

mutex second_thread_tried_to_acquire_mutex;
condition_variable is_second_thread_tried_to_acquire_cond;
bool second_thread_tried_to_acquire_pred = false;

mutex first_thread_released_mutex;
condition_variable is_first_thread_released_cond;
bool first_thread_released_pred = false;

void do_acquire() {
    // we need to ensure that first thread acquired a lock before we call try_lock() func
    unique_lock<mutex> first_thread_acquired_unique_lock(first_thread_acquired_mutex);
    cout << "Thread 2 is waiting until the lock is acquired by Thread 1" << endl;
    is_first_thread_acquired_cond.wait(first_thread_acquired_unique_lock, [] { return first_thread_acquired_pred; });

    cout << "Thread 2 got a notify and trying to acquire a lock while Thread 1 is holding it" << endl;
    bool locked = lock.try_lock();
    REQUIRE(!locked);
    cout << "Thread 2 tried to acquire a lock | Result: " + to_string(locked) << endl;

    {
        lock_guard<mutex> lockGuard(second_thread_tried_to_acquire_mutex);
        second_thread_tried_to_acquire_pred = true;
        cout << "Thread 2 is notifying that try was done so Thread 1 can work further" << endl;
    }
    is_second_thread_tried_to_acquire_cond.notify_one();

    cout << "Thread 2 is waiting until the lock is released by Thread 1" << endl;
    unique_lock<mutex> first_thread_released_unique_lock(first_thread_released_mutex);
    is_first_thread_released_cond.wait(first_thread_released_unique_lock, [] { return first_thread_released_pred; });

    cout << "Thread 2 got a notify and trying to acquire a lock while Thread 1 released it" << endl;
    locked = lock.try_lock();
    cout << "Thread 2 tried to acquire a lock | Result: " + to_string(locked) << endl;
    REQUIRE(locked);

    lock.release();
}

void find_n_prime_number(int n) {
    lock.acquire();
    cout << "Thread 1 acquired a lock" << endl;
    {
        lock_guard<mutex> lockGuard(first_thread_acquired_mutex);
        first_thread_acquired_pred = true;
        cout << "Thread 1 is notifying that lock was acquired so Thread 2 can try to acquire a lock" << endl;
    }
    is_first_thread_acquired_cond.notify_one();

    unique_lock<mutex> second_thread_tried_to_acquire_unique_lock(second_thread_tried_to_acquire_mutex);
    cout << "Thread 1 is waiting until Thread 2 try to acquire a lock" << endl;
    is_second_thread_tried_to_acquire_cond.wait(second_thread_tried_to_acquire_unique_lock,
                                                [] { return second_thread_tried_to_acquire_pred; });

    cout << "Thread 1 got a notify and started calculating n-th prime number" << endl;

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
        current_num++;
    }

    cout << "Thread 1 found n-th prime number" << endl;

    lock.release();
    cout << "Thread 1 released a lock" << endl;
    {
        lock_guard<mutex> lockGuard(first_thread_released_mutex);
        first_thread_released_pred = true;
        cout << "Thread 1 is notifying that lock was released so Thread 2 can try to acquire a lock again" << endl;
    }
    is_first_thread_released_cond.notify_one();
}

TEST_CASE("Unable acquire a lock while other thread already acquired it") {
    THREADS_NUM = 2;

    vector<thread> threads;
    threads.reserve(THREADS_NUM);

    threads.emplace_back(thread(do_acquire)); // put second thread to work
    threads.emplace_back(thread(find_n_prime_number, 50000)); // put first thread to work (long work)

    for (int i = 0; i < THREADS_NUM; i++) {
        threads[i].join();
    }
}