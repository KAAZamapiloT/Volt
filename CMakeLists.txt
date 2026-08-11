cmake_minimum_required(VERSION 3.20)

project(Volt
    VERSION 0.1.0
    LANGUAGES CXX
)

add_library(Volt INTERFACE)

target_compile_features(Volt INTERFACE cxx_std_20)

target_include_directories(Volt
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

option(VOLT_BUILD_EXAMPLES "Build Volt examples" ON)

if(VOLT_BUILD_EXAMPLES)
    add_executable(volt_example
        examples/basic.cpp
    )

    target_link_libraries(volt_example
        PRIVATE Volt
    )
endif()

option(VOLT_BUILD_TESTS "Build Volt tests" ON)

if(VOLT_BUILD_TESTS)
    enable_testing()

    add_executable(volt_tests
        tests/basic.cpp
    )

    target_link_libraries(volt_tests
        PRIVATE Volt
    )

    add_test(
        NAME volt_tests
        COMMAND volt_tests
    )
endif()