cmake_minimum_required(VERSION 3.18)
project(MemPool)

set(CMAKE_CXX_STANDARD 17)

include_directories("MemPool/include")
add_library(MemPool SHARED src/Base/ThreadInfo.cpp src/MemPool.cpp)

add_executable(ThreadPoolTest test/MemPoolTest.cpp)
include_directories("MemPool/include")
include_directories("MemPool/include/test")
target_link_libraries (ThreadPoolTest MemPool pthread)
