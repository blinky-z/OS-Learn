#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include "Spinlock.h"
using namespace std;

Spinlock lock;
int cnt = 0;

const int THREADS_NUM = 2;

void thread_run(int thread_num) {
    for (int i = 0; i < 100; i++) {
        lock.acquire();
        cnt++;
        cout << "[Thread " << thread_num << "] " << cnt << endl;
        lock.release();
    }
}

int main() {
    vector<thread> threads;
    threads.reserve(THREADS_NUM);
    for (int i = 0; i < THREADS_NUM; i++) {
        threads.emplace_back(thread(thread_run, i));
    }

    for (int i = 0; i < THREADS_NUM; i++) {
        threads[i].join();
    }

    assert(cnt == 200);
}