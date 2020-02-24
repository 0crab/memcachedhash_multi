cmake_minimum_required(VERSION 3.10)
project(memcachedhash C)

set(CMAKE_C_STANDARD 99)

link_libraries(pthread)

add_executable(memcachedhash main.c assoc.c jenkins_hash.c my_memcached.c)
