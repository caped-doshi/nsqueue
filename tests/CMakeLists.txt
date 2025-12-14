include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)

FetchContent_MakeAvailable(Catch2)

add_executable(spsc_test
    spsc_test.cc
)

target_link_libraries(spsc_test
    PRIVATE
        nsqueue
        Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(spsc_test)
