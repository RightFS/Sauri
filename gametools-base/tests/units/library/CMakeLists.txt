# @file CMakeLists.txt
# @brief CMake configuration for unit tests
# @copyright Copyright (c) 2025 Leigod Technology Co., Ltd. All rights reserved.
#
# @note This file is part of the Leigod Library Manager.
# Created: 2025-03-31
# Author: chenxu

function(set_nn_game_library_include target)
    target_include_directories(${target}
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/library
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/library/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/library/task
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/library/observer
    )
endfunction()

add_nn_game_test(libraryMetaDataTest metaDataTest.cpp)
add_nn_game_test(libraryInstallOptionTest installOptionTest.cpp)
add_nn_game_test(libraryManagerTest managerTest.cpp)
add_nn_game_test(libraryTaskManagerTest taskManagerTest.cpp)
add_nn_game_test(libraryScriptTaskTest scriptTaskTest.cpp)
add_nn_game_test(libraryPipeTaskTest pipeTaskTest.cpp)
add_nn_game_test(libraryDBTaskManagerTest dbTaskManagerTest.cpp)
add_nn_game_test(libraryDBManagerTest dbManagerTest.cpp)

set_nn_game_library_include(libraryManagerTest)
set_nn_game_library_include(libraryTaskManagerTest)
set_nn_game_library_include(libraryScriptTaskTest)
set_nn_game_library_include(libraryPipeTaskTest)
set_nn_game_library_include(libraryDBTaskManagerTest)
set_nn_game_library_include(libraryDBManagerTest)