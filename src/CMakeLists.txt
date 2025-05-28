cmake_minimum_required(VERSION 3.15)
project(sauri)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(boost_asio REQUIRED CONFIG)
find_package(stduuid CONFIG REQUIRED)
find_package(CMakeRC CONFIG REQUIRED)

aux_source_directory(. SOURCE_FILES)
aux_source_directory(rpc SOURCE_FILES)
aux_source_directory(rpc/pipe SOURCE_FILES)
aux_source_directory(res_processor SOURCE_FILES)



cmrc_add_resource_library(resources-dummy
        ALIAS res::dummy
        NAMESPACE res::dummy
        cmrc.keep
)
# 定义输出目标
add_library(${PROJECT_NAME}
        ${SOURCE_FILES}
)

# 可能需要链接额外的库
target_link_libraries(${PROJECT_NAME} PUBLIC
        Boost::asio
        common
        stduuid
        res::dummy
)

target_compile_definitions(${PROJECT_NAME} PUBLIC -D_WIN32_WINNT=0x0601)
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /wd4996 /wd4100 /wd5054 /wd4020 /wd4018 /wd4200 /wd4459 /wd4389")
    include(${NNGAME_ROOT_PATH}/cmake/properties/msvc.cmake)
endif ()