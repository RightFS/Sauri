# @file CMakeLists.txt
# @brief CMake configuration for unit tests
# @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
#
# @note This file is part of the Leigod Unit Test Framework.
# Created: 2025-03-31
# Author: chenxu

option(ENABLE_LIBRARY_TESTS "Enable library tests" ON)

if (ENABLE_LIBRARY_TESTS)
    add_subdirectory(library)
endif ()
