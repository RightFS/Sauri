project(game_tool_base)
option(ENABLE_TESTS "Enable unit tests" ON)

include(${NNGAME_ROOT_PATH}/cmake/defined.cmake)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(${NNGAME_ROOT_PATH}/cmake/platforms/darwin.cmake)
elseif (CMAKE_SYSTEM_NAME MATCHES "iOS")
    include(${NNGAME_ROOT_PATH}/cmake/platforms/iOS.cmake)
elseif (CMAKE_SYSTEM_NAME MATCHES "Android")
    include(${NNGAME_ROOT_PATH}/cmake/platforms/android.cmake)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    include(${NNGAME_ROOT_PATH}/cmake/platforms/linux.cmake)
elseif (MSVC)
    include(${NNGAME_ROOT_PATH}/cmake/platforms/msvc.cmake)
else ()
    message(FATAL_ERROR "Unsupported platform")
endif ()

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
add_compile_definitions(NNGAME_EXPORTS)


aux_source_directory(src SOURCE_FILES)
aux_source_directory(src/downloader SOURCE_FILES)

#add_library(${PROJECT_NAME} SHARED
#        ${SOURCE_FILES}
#)
find_package(rpclib CONFIG REQUIRED)

add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)
target_link_libraries(${PROJECT_NAME}
        PUBLIC
        rpclib::rpc
        CURL::libcurl
        OpenSSL::Crypto
)

if (ENABLE_TESTS)
    add_subdirectory(tests)
endif ()

if (MSVC)
    include(${NNGAME_ROOT_PATH}/cmake/properties/msvc.cmake)
endif ()