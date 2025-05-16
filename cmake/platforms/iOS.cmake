include(${NNGAME_ROOT_PATH}/cmake/platforms/darwin.cmake)

set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
add_custom_target(xcframework
    COMMAND "${NNGAME_ROOT_PATH}/ios_framework.sh"
    WORKING_DIRECTORY "${NNGAME_ROOT_PATH}"
    DEPENDS leigod_im_cross_sdk
    COMMENT "build framework for ios"
)

