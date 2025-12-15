include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)

FetchContent_MakeAvailable(Catch2)

add_executable(spsc_unit_tests
    spsc_test.cc
)

target_link_libraries(spsc_unit_tests
    PRIVATE nsqueue Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(
    spsc_unit_tests
    TEST_SPEC "[unit]"
    PROPERTIES LABELS unit
)

add_executable(spsc_stress_tests
    spsc_test.cc
)

target_link_libraries(spsc_stress_tests
    PRIVATE nsqueue Catch2::Catch2WithMain
)

catch_discover_tests(
    spsc_stress_tests
    TEST_SPEC "[stress]"
    PROPERTIES LABELS stress
)
