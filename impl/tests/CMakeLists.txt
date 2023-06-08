cmake_minimum_required(VERSION 3.12)
project(mutex_tests)
project(spinlock_tests)

set(CMAKE_CXX_STANDARD 11)

add_executable(mutex_tests
        ../mutex/Mutex.h ../mutex/Mutex.cpp
        mutex_tests.cpp
        )

add_executable(spinlock_tests
        ../spinlock/Spinlock.h ../spinlock/Spinlock.cpp
        spinlock_tests.cpp
        )

target_link_libraries(mutex_tests pthread)
target_link_libraries(spinlock_tests pthread)
