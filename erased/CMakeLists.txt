add_library(erased INTERFACE)

target_compile_features(erased INTERFACE cxx_std_23)
target_include_directories(erased INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_sources(erased
    PUBLIC
        FILE_SET erased_headers
        TYPE HEADERS
        BASE_DIRS ./include/
        FILES include/erased/erased.h
)

add_library(erased::erased ALIAS erased)
