project(nngame_game_sdk)


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

aux_source_directory(src SOURCE_FILES)

#add_library(${PROJECT_NAME} SHARED
#        ${SOURCE_FILES}
#)
find_package(rpclib CONFIG REQUIRED)


add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        rpclib::rpc
)
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /wd4996 /wd4100 /wd5054 /wd4020 /wd4018 /wd4200 /wd4459 /wd4389")
    include(${NNGAME_ROOT_PATH}/cmake/properties/msvc.cmake)
endif ()