cmake_minimum_required(VERSION 3.10)
project(process_lib)

set(CMAKE_CXX_STANDARD 17)

add_library(process_runner
    src/process_runner.cpp
)

target_include_directories(process_runner PUBLIC include)

add_executable(test_runner
    test/test_runner.cpp
)

target_link_libraries(test_runner process_runner)
