# Copyright (c) 2025 Leigod Inc.
# Author: chenxu
# Date: 2025-04-01
# 
# CMake file for the source directory

set(COMMON_SOURCES)

# Define the source directories
add_subdirectory(task)

message(STATUS "COMMON_SOURCES:${COMMON_SOURCES}")


add_library(leigod_common STATIC
        ${COMMON_SOURCES}
)

set_common_properties(leigod_common)