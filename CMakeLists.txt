cmake_minimum_required(VERSION 3.22)
project(nsqueue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NSQUEUE_BUILT_TESTS "Built unit tests" ON)
option(NSQUEUE_BUILD_BENCHMARKS "Build benchmarks" ON)

# Header only library
add_library(nsqueue INTERFACE)

target_include_directories(nsqueue
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(NSQUEUE_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

if(NSQUEUE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

