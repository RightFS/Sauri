# Copyright (c) 2025 Leigod Inc.
# Author: chenxu
# Date: 2025-04-01
# 
# CMake file for the source directory

SET(TASK_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/task.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/task_manager.cpp
)

LIST(APPEND COMMON_SOURCES ${TASK_SOURCES})

set(COMMON_SOURCES ${COMMON_SOURCES} PARENT_SCOPE)