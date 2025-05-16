set(NNIM_BUILD_SHARED OFF) #android 不编译动态库

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-D_DEBUG)
endif ()

add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Oz -ffunction-sections -fdata-sections -fvisibility=hidden -flto")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Oz -ffunction-sections -fdata-sections -fvisibility=hidden -flto")