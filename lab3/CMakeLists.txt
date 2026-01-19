cmake_minimum_required(VERSION 3.15)
project(CounterApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC
    src/main.cpp
    src/counter.cpp
    src/logger.cpp
    src/spawn.cpp
)

add_executable(CounterApp ${SRC})

target_include_directories(CounterApp
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

if(UNIX)
    target_link_libraries(CounterApp pthread)
    # macOS не использует librt, Linux — можно добавить
    if(NOT APPLE)
        target_link_libraries(CounterApp rt)
    endif()
endif()
