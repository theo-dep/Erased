FetchContent_Declare(googletest
GIT_REPOSITORY https://github.com/google/googletest
GIT_TAG v1.15.2)

FetchContent_Declare(googlebench
GIT_REPOSITORY https://github.com/google/benchmark
GIT_TAG v1.9.1
)

FetchContent_MakeAvailable(googletest googlebench)

add_executable(Benchmarks benchmarks.cpp)
target_link_libraries(Benchmarks PRIVATE erased benchmark::benchmark)

