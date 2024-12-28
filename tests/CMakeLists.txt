FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG v1.15.2
)

FetchContent_MakeAvailable(googletest)

add_executable(CompileTimeTests compile_time_tests.cpp)
target_link_libraries(CompileTimeTests PRIVATE erased::erased)

add_executable(Tests tests.cpp)
target_link_libraries(Tests PRIVATE erased::erased gtest gtest_main)

add_test(NAME Tests COMMAND Tests)
