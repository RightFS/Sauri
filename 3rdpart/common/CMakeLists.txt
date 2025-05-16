cmake_minimum_required(VERSION 3.16)
project(leigod_common VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type install_options
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()

# 设置项目版本号
set(COMMON_VERSION_MAJOR 1)
set(COMMON_VERSION_MINOR 0)
set(COMMON_VERSION_PATCH 0)

# 创建版本头文件的配置
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in"
        "${CMAKE_CURRENT_BINARY_DIR}/include/common/version.hpp"
)

# Options
option(BUILD_TESTS "Build test suite" OFF)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers in debug builds" OFF)

# Define the install_options to control the compilation of the example directory
option(ENABLE_EXAMPLES "Enable example directory compilation" OFF)

function(set_common_properties target)
    target_compile_options(${target}
            PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /utf-8 /wd4100>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror -pedantic>
    )

    target_compile_definitions(${target} PRIVATE
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
            _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS
            _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    )

    target_include_directories(${target}
            PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    install(TARGETS ${target}
            EXPORT common-targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/common
    )
endfunction()

# Coverage flags
if (ENABLE_COVERAGE)
    if (NOT MSVC)
        add_compile_options(-fprofile-arcs -ftest-coverage)
        add_link_options(-fprofile-arcs -ftest-coverage)
    endif ()
endif ()

# Sanitizer flags
if (ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (NOT MSVC)
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif ()
endif ()

# Platform specific configurations
if (WIN32)
    add_definitions(-DLEIGOD_PLATFORM_WINDOWS)
elseif (APPLE)
    add_definitions(-DLEIGOD_PLATFORM_MACOS)
elseif (UNIX)
    add_definitions(-DLEIGOD_PLATFORM_LINUX)
endif ()

add_subdirectory(src)

# Add example directory if the install_options is enabled
if (ENABLE_EXAMPLES)
    #    add_subdirectory(example)
endif ()

# Generate export header
include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
        BASE_NAME LEIGOD_COMMON
        EXPORT_MACRO_NAME LEIGOD_EXPORT
        EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/common/core/export.hpp
)

# Installation
install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)


install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp"
)

# Testing
if (BUILD_TESTS)
    #    enable_testing()
    #    add_subdirectory(tests)
endif ()

# Documentation
find_package(Doxygen)
if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(common_docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
    )
endif ()