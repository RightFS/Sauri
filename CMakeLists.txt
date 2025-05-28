# CMakeList.txt : CMake project for nngame, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.16)


# Enable Hot Reload for MSVC compilers if supported.
#if (POLICY CMP0141)
#  cmake_policy(SET CMP0141 NEW)
#  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
#endif()

project("game-tool-base" VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_TESTS "Enable unit tests" ON)
option(ENABLE_EXAMPLES "Enable examples" ON)

# Add source to this project's executable.
#add_executable (nngame "nngame.cpp" "nngame.h" "Common/include/singleton.h"
#        gametools-base/src/test1.cpp
#        gametools-base/src/test1.h)

#if (CMAKE_VERSION VERSION_GREATER 3.12)
#  set_property(TARGET nngame PROPERTY CXX_STANDARD 20)
#endif()

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# set path to the cmake folder
set(NNGAME_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "")
add_subdirectory(common)
#add_subdirectory(gametools-base)
#add_subdirectory(gametools-node)
#add_subdirectory(3rdpart)
add_subdirectory(examples)
add_subdirectory(src)