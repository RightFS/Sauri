cmake_minimum_required(VERSION 3.15)
project(sauri)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(boost_asio REQUIRED CONFIG)
find_package(stduuid CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(easyloggingpp easyloggingpp REQUIRED IMPORTED_TARGET)

aux_source_directory(. SOURCE_FILES)
aux_source_directory(rpc SOURCE_FILES)
aux_source_directory(rpc/pipe SOURCE_FILES)
aux_source_directory(logger_helper SOURCE_FILES)

# 定义输出目标
add_library(${PROJECT_NAME}
        ${SOURCE_FILES}
)

# Set public header path
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
# 可能需要链接额外的库
target_link_libraries(${PROJECT_NAME} PUBLIC
        Boost::asio
        stduuid
        PkgConfig::easyloggingpp
)

target_compile_definitions(${PROJECT_NAME} PUBLIC -D_WIN32_WINNT=0x0601)
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            /utf-8 /wd4996 /wd4100 /wd5054 /wd4020 /wd4018 /wd4200 /wd4459 /wd4389)
    include(${CMAKE_SOURCE_DIR}/cmake/properties/msvc.cmake)
endif ()


# Install targets
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Export targets
install(EXPORT ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}-targets.cmake
        NAMESPACE sauri::
        DESTINATION share/${PROJECT_NAME}
)

# Generate and install config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
        ${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
        INSTALL_DESTINATION share/${PROJECT_NAME}
)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
        VERSION 0.0.1
        COMPATIBILITY SameMajorVersion
)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
        DESTINATION share/${PROJECT_NAME}
)