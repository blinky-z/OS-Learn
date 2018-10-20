#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include "../Spinlock.h"
using namespace std;

Spinlock lock;
int cnt = 0;

const int THREADS_NUM = 100;
const int OP_PER_THREAD = 100;

void reset_count() {
    cnt = 0;
}

void thread_run(int thread_num) {
    for (int i = 0; i < OP_PER_THREAD; i++) {
        lock.acquire();
        cnt++;
        cout << "[Thread " << thread_num << "] " << cnt << endl;
        lock.release();
    }
}

TEST_CASE("Concurrent count increment") {
    for (int current_try = 0; current_try < 10; current_try++) {
        vector<thread> threads;
        threads.reserve(THREADS_NUM);
        for (int i = 0; i < THREADS_NUM; i++) {
            threads.emplace_back(thread(thread_run, i));
        }

        for (int i = 0; i < THREADS_NUM; i++) {
            threads[i].join();
        }

        REQUIRE(cnt == THREADS_NUM * OP_PER_THREAD);
        reset_count();
    }
}