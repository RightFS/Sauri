if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-D_DEBUG)
    add_compile_definitions(DEBUG=1)
endif ()

add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

# 设置生成dSYM文件
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")

set(PUBLIC_HEADERS
    ${PUBLIC_HEADERS}
    ${LEIGOD_IM_SDK_SRC_DIR}/darwin/imsdk.h
)

set (APPLE_HEADER_LIST
    ${LEIGOD_IM_SDK_SRC_DIR}/darwin/configuration
    ${LEIGOD_IM_SDK_SRC_DIR}/darwin/configuration/group
)
list(APPEND LEIGOD_IM_SDK_HEADER ${APPLE_HEADER_LIST})
message("LEIGOD_IM_SDK_HEADER: ${LEIGOD_IM_SDK_HEADER}")

set(APPLE_FILE_LIST
    ${LEIGOD_IM_SDK_SRC_DIR}/darwin/imsdk.mm
    ${LEIGOD_IM_SDK_SRC_DIR}/darwin/httplib.mm
    ${LEIGOD_IM_SDK_SRC_DIR}/darwin/Reachability.m
    ${LEIGOD_IM_SDK_SRC_DIR}/darwin/IMSystemInterface.mm
)

list(APPEND SOURCE_FILES ${APPLE_FILE_LIST})

file(GLOB_RECURSE APPLE_OBJC_FILES LIST_DIRECTORIES false ${LEIGOD_IM_SDK_SRC_DIR}/darwin/configuration/*.m ${LEIGOD_IM_SDK_SRC_DIR}/darwin/configuration/*.mm)
list(APPEND SOURCE_FILES ${APPLE_OBJC_FILES})
message("Objective-C/C++ files: ${APPLE_OBJC_FILES}")

file(GLOB_RECURSE APPLE_OBJC_HEADERS LIST_DIRECTORIES false ${LEIGOD_IM_SDK_SRC_DIR}/darwin/configuration/*.h)
list(APPEND PUBLIC_HEADERS ${APPLE_OBJC_HEADERS})
message("Objective-C/C++ headers: ${PUBLIC_HEADERS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fobjc-arc -Werror=return-type")

find_library(SECURITY_LIBRARY Security)
find_library(COREFOUNDATION_LIBRARY CoreFoundation)
find_library(FOUNDATION_LIBRARY Foundation)
#SystemConfiguration.framework
find_library(SYSTEMCONFIGURATION_LIBRARY SystemConfiguration)

if (NOT COREFOUNDATION_LIBRARY)
    message(FATAL_ERROR "CoreFoundation not found")
endif ()

if (NOT FOUNDATION_LIBRARY)
    message(FATAL_ERROR "Foundation not found")
endif ()

# 平台相关的依赖库
set(LIMSDK_PLATFORM_LIB 
    ${COREFOUNDATION_LIBRARY}
    ${FOUNDATION_LIBRARY}
    ${SECURITY_LIBRARY}
    ${SYSTEMCONFIGURATION_LIBRARY}
)
