#if(EXISTS "${CURRENT_INSTALLED_DIR}/share/libressl/copyright"
#    OR EXISTS "${CURRENT_INSTALLED_DIR}/share/boringssl/copyright")
#    message(FATAL_ERROR "Can't build openssl if libressl/boringssl is installed. Please remove libressl/boringssl, and try install openssl again if you need it.")
#endif()

vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO Tencent/sqlcipher
        REF 5d8825c22feedb421ad6f9ecfc1399460e10d299
        HEAD_REF wcdb
        SHA512 f08017e9b0b5405b953ebfaac00a5524cb4fc9e3f3f2cfb21dd988290c20713d97336bf2ddd2ff3f861256cb230a9a3fb9eef441ffd0b410b1c40613371dbf2a
        PATCHES
        patches/fix-sqlcipher.patch
)

# 检查是否存在 sqlcipher.cmake（需根据实际路径验证）
vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME "sqlcipher")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")