#if(EXISTS "${CURRENT_INSTALLED_DIR}/share/libressl/copyright"
#    OR EXISTS "${CURRENT_INSTALLED_DIR}/share/boringssl/copyright")
#    message(FATAL_ERROR "Can't build openssl if libressl/boringssl is installed. Please remove libressl/boringssl, and try install openssl again if you need it.")
#endif()

vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO RightFS/sauri
        REF 8b1c138ed40f2bc8cedb983598e22a0a8e83e036
        HEAD_REF master
        SHA512 ac747cfe2d0c517468b55112355720d0a93f4a19977466fbdacd2c85060b61d60fc7aa87606400b7f4d45eb734f893135285e4a3a14d0e0b422ad89682a23f54
)

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
        -DENABLE_EXAMPLES=OFF
        -DENABLE_TESTS=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH share/sauri)
vcpkg_copy_pdbs()

# Remove debug include directories - THIS FIXES YOUR ERROR
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

#file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
#vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
