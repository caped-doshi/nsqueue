include(FetchContent)
FetchContent_Declare(
    nanobench
    GIT_REPOSITORY https://github.com/martinus/nanobench.git
    GIT_TAG v4.3.11
)

FetchContent_MakeAvailable(nanobench)

add_executable(spsc_bench spsc_bench.cc)

target_link_libraries(spsc_bench
    PRIVATE
        nsqueue
        nanobench
)
