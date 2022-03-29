cmake_minimum_required(VERSION 3.12)
project(MemPool)

set(CMAKE_CXX_STANDARD 17)

include_directories("MemPool/include")
add_library(MemPool SHARED src/Base/ThreadInfo.cpp src/MemPool.cpp)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize-undefined-trap-on-error -fsanitize=bounds-strict -fstack-protector-all -fstack-clash-protection")
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize-undefined-trap-on-error -fsanitize=bounds-strict -fstack-protector-all -fstack-clash-protection")

add_executable(MemPoolTest test/MemPoolTest.cpp)
include_directories("MemPool/include")
include_directories("MemPool/include/test")
target_link_libraries(MemPoolTest MemPool pthread)
