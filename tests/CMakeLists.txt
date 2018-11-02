cmake_minimum_required(VERSION 3.12)
project(mutex_tests)
project(spinlock_tests)

set(CMAKE_CXX_STANDARD 11)

add_executable(mutex_tests
#        src files
        ../Mutex.h ../Mutex.cpp
#        ------------------------
        tests.cpp
        )

add_executable(spinlock_tests
        #        src files
        ../Spinlock.h ../Spinlock.cpp
        #        ------------------------
        #        tests
        tests.cpp
        )

target_link_libraries(mutex_tests pthread)
target_compile_definitions(mutex_tests PRIVATE TEST_MUTEX=1)
target_link_libraries(spinlock_tests pthread)
target_compile_definitions(spinlock_tests PRIVATE TEST_SPINLOCK=1)