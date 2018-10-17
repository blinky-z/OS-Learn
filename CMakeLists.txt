cmake_minimum_required(VERSION 3.12)
project(ThreadsLearn)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

add_executable(ThreadsLearn
#        Spinlock
        Spinlock.cpp Spinlock.h)

target_link_libraries(ThreadsLearn pthread)