if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-D_DEBUG)
endif ()

add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

# 平台相关的依赖库
set(LIMSDK_PLATFORM_LIB uuid)