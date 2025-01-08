cmake_minimum_required(VERSION 3.30)
include(FetchContent)

project(erased LANGUAGES CXX)

add_subdirectory(erased)
add_subdirectory(tests)
add_subdirectory(benchmarks)
