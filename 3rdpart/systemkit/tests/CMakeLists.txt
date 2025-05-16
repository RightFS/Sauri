
function(add_test target sources)
    add_executable(${target} ${sources})
    target_link_libraries(${target}
            PRIVATE
            ${PROJECT_NAME}
            gtest
            gtest_main
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


# Google Test dependency
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)
# Enable testing
include(GoogleTest)

add_test(registry_manager_test units/registry_manager_test.cpp)
add_test(firewall_controller_test units/firewall_controller_test.cpp)
add_test(environment_variables_test units/environment_variables_test.cpp)
add_test(environment_variables_chinese_test units/environment_variables_chinese_test.cpp)
add_test(process_launcher_test units/process_launcher_test.cpp)