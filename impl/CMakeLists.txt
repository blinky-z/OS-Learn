cmake_minimum_required(VERSION 3.12)
project(os_learn)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

add_subdirectory(tests)
add_executable(os_learn
        spinlock/Spinlock.cpp spinlock/Spinlock.h
        mutex/Mutex.cpp mutex/Mutex.h
        )

target_link_libraries(os_learn pthread)
