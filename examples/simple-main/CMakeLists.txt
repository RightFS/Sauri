cmake_minimum_required(VERSION 3.15)
project(simple-main)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

aux_source_directory(. SOURCE_FILES)
# 定义输出目标
add_executable(${PROJECT_NAME}
        ${SOURCE_FILES}
)

# 可能需要链接额外的库
target_link_libraries(${PROJECT_NAME} PRIVATE
        game_tool_base
        common
)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /wd4996 /wd4100 /wd5054 /wd4020 /wd4018 /wd4200 /wd4459 /wd4389")
    include(${NNGAME_ROOT_PATH}/cmake/properties/msvc.cmake)
endif ()