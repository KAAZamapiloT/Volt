cmake_minimum_required(VERSION 3.20)

project(Volt
    VERSION 0.1.0
    LANGUAGES CXX
)

add_library(Volt INTERFACE)

target_compile_features(Volt
    INTERFACE
        cxx_std_23
)

target_include_directories(Volt
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Main executable
add_executable(volt
    main.cpp
)

target_link_libraries(volt
    PRIVATE Volt
)

# Tests / benchmarks
option(VOLT_BUILD_TESTS "Build Volt tests" ON)

if(VOLT_BUILD_TESTS)

    enable_testing()

    add_executable(volt_tests
        tests/basic.cpp
    )

    target_link_libraries(volt_tests
        PRIVATE Volt
    )

    target_include_directories(volt_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    add_test(
        NAME volt_tests
        COMMAND volt_tests
    )

endif()