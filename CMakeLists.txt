cmake_minimum_required(VERSION 3.8)
project(OSHW2)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        mm_alloc.c
        mm_alloc.h
        mm_test.c)

add_executable(OSHW2 ${SOURCE_FILES})