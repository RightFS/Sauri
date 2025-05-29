cmake_minimum_required(VERSION 3.15)
project(simple-main)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(CLI11 CONFIG REQUIRED)
find_package(CMakeRC CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(easyloggingpp easyloggingpp REQUIRED IMPORTED_TARGET)

cmrc_add_resource_library(resources-asar
        ALIAS res::asar
        NAMESPACE res::asar
        cmrc.test
)

aux_source_directory(. SOURCE_FILES)
aux_source_directory(./res_processor SOURCE_FILES)
# 定义输出目标
add_executable(${PROJECT_NAME}
        ${SOURCE_FILES}
)

# 可能需要链接额外的库
target_link_libraries(${PROJECT_NAME} PRIVATE
        sauri
        res::asar
        CLI11::CLI11
        PkgConfig::easyloggingpp
)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /wd4996 /wd4100 /wd5054 /wd4020 /wd4018 /wd4200 /wd4459 /wd4389")
    include(${CMAKE_SOURCE_DIR}/cmake/properties/msvc.cmake)
endif ()