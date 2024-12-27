cmake_minimum_required(VERSION 3.20)
include(FetchContent)

project(erased LANGUAGES CXX)

add_subdirectory(erased)

#FetchContent_Declare(googletest
#GIT_REPOSITORY https://github.com/google/googletest
#GIT_TAG v1.15.2)

#FetchContent_Declare(googlebench
#GIT_REPOSITORY https://github.com/google/benchmark
#GIT_TAG v1.9.1
#)

#FetchContent_MakeAvailable(googletest googlebench)

add_subdirectory(examples)
add_subdirectory(tests)
