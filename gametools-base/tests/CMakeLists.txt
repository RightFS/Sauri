# @file CMakeLists.txt
# @brief CMake configuration for unit tests
# @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
#
# @note This file is part of the Leigod Unit Test Framework.
# Created: 2025-03-31
# Author: chenxu

cmake_minimum_required(VERSION 3.16)
project(unit_tests)

find_package(GTest CONFIG REQUIRED)

enable_testing()

option(ENABLE_UNITS_TESTS "Enable unit tests" ON)

set(MIN_TESTS_SRC
        test.cpp
)

aux_source_directory(. ALL_TESTS_SRC)
aux_source_directory(./downloader ALL_TESTS_SRC)

add_executable(
        min_tests
        ${MIN_TESTS_SRC}
)

add_executable(
        all_tests
        ${ALL_TESTS_SRC}
)

target_link_libraries(
        min_tests
        PRIVATE
        nngame_client_sdk
        GTest::gtest GTest::gtest_main
)

target_link_libraries(
        all_tests
        PRIVATE
        nngame_client_sdk
        GTest::gtest GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(min_tests PROPERTIES LABELS "min")
gtest_discover_tests(all_tests PROPERTIES LABELS "all")

#add_test(AllTestsInMin min_tests)
#add_test(AllTestsInAll all_tests)

set_property(TARGET all_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET min_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

function(add_nn_game_test target sources)
    add_executable(${target} ${sources})
    target_link_libraries(${target}
            PRIVATE
            nngame_client_sdk
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
    )

    target_compile_definitions(${target} PRIVATE
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
            _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS
            _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    )

    target_compile_options(${target}
            PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /utf-8 /wd4100>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror -pedantic>
    )

    gtest_discover_tests(${target})

    # Add test coverage target
    if (ENABLE_COVERAGE AND NOT MSVC)
        add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
                COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}
                gcovr -r ${CMAKE_SOURCE_DIR}
                --html --html-details
                -o coverage/index.html
                --exclude-unreachable-branches
                --exclude-throw-branches
                DEPENDS ${target}
                COMMENT "Generating coverage report"
        )
    endif ()

endfunction()

if (ENABLE_UNITS_TESTS)
    add_subdirectory(units)
endif ()
