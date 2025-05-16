set(NNIM_PRODUCT_MAJOR 2)
set(NNIM_PRODUCT_MINOR 1)
set(NNIM_PRODUCT_MICRO 5)
set(NNIM_PRODUCT_BUILD 64)
add_definitions(-DNNIM_PRODUCT_MAJOR=${NNIM_PRODUCT_MAJOR})
add_definitions(-DNNIM_PRODUCT_MINOR=${NNIM_PRODUCT_MINOR})
add_definitions(-DNNIM_PRODUCT_MICRO=${NNIM_PRODUCT_MICRO})
add_definitions(-DNNIM_PRODUCT_BUILD=${NNIM_PRODUCT_BUILD})

add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
add_definitions(-DCPPHTTPLIB_ZLIB_SUPPORT)
add_definitions(-DCPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN)
add_definitions(-DSPDLOG_WCHAR_FILENAMES)
add_definitions(-DASIO_HAS_CO_AWAIT)
add_definitions(-DSPDLOG_ACTIVE_LEVEL=0)
add_definitions(-DCPPHTTPLIB_BROTLI_SUPPORT)

set(LEIGOD_IM_SDK_THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
message("LEIGOD_IM_SDK_THIRD_PARTY_DIR = ${LEIGOD_IM_SDK_THIRD_PARTY_DIR}")

set(LEIGOD_IM_SDK_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
message("LEIGOD_IM_SDK_SRC_DIR = ${LEIGOD_IM_SDK_SRC_DIR}")

set(LEIGOD_IM_SDK_OUT_DIR ${CMAKE_SOURCE_DIR}/build)
message("LEIGOD_IM_SDK_OUT_DIR = ${LEIGOD_IM_SDK_OUT_DIR}")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TARGET_ARCH "64")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(TARGET_ARCH "32")
endif ()
message(STATUS "Current build: ${CMAKE_SYSTEM_NAME}-${TARGET_ARCH}")

if (OUTPUT)
    set(LEIGOD_IM_SDK_OUT_DIR ${LEIGOD_IM_SDK_OUT_DIR}/${OUTPUT})
else ()
    set(LEIGOD_IM_SDK_OUT_DIR ${LEIGOD_IM_SDK_OUT_DIR}/${CMAKE_SYSTEM_NAME}_${TARGET_ARCH})
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LEIGOD_IM_SDK_OUT_DIR} CACHE PATH "Output directory for archive files")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LEIGOD_IM_SDK_OUT_DIR} CACHE PATH "Output directory for library files")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LEIGOD_IM_SDK_OUT_DIR} CACHE PATH "Output directory for executable files")
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${LEIGOD_IM_SDK_OUT_DIR} CACHE PATH "Output directory for debug archive files")
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${LEIGOD_IM_SDK_OUT_DIR} CACHE PATH "Output directory for release archive files")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${LEIGOD_IM_SDK_OUT_DIR} CACHE PATH "Output directory for debug library files")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${LEIGOD_IM_SDK_OUT_DIR} CACHE PATH "Output directory for release library files")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${LEIGOD_IM_SDK_OUT_DIR} CACHE PATH "Output directory for debug executable files")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${LEIGOD_IM_SDK_OUT_DIR} CACHE PATH "Output directory for release executable files")

if (WIN32)
    add_definitions(-DLEIGOD_PLATFORM_WINDOWS)
elseif (APPLE)
    add_definitions(-DLEIGOD_PLATFORM_MACOS)
elseif (UNIX)
    add_definitions(-DLEIGOD_PLATFORM_LINUX)
endif ()

function(set_nn_game_properties target)
    target_compile_options(${target}
            PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /utf-8 /wd4100>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror -pedantic>
    )

    target_compile_definitions(${target} PRIVATE
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
            _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS
            _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    )

    target_include_directories(${target}
            PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    install(TARGETS ${target}
            EXPORT nngame-targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nngame
    )
endfunction()
