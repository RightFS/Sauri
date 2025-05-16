# portfile.cmake

# 下载源码
vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO Tencent/wcdb
        REF v2.1.10

        SHA512 b537dbe1d87fbca8f0a2414cebe387dba51a961c2f3df08b6d4d3c42b12cae8790cb9f8e714072f85fa16c00f575e8b8fe5cdee72713306952494963a1aaff36
        PATCHES "${CMAKE_CURRENT_LIST_DIR}/patches/fix-wcdb.patch"
)

# 配置 CMake 选项
vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        MAYBE_UNUSED_VARIABLES
        OPTIONS
        -DBUILD_TESTS=OFF  # 默认关闭测试（需匹配 WCDB 的实际 CMake 选项）
        -DBUILD_SHARED_LIBS=OFF
)

# 编译并安装
vcpkg_cmake_install()
vcpkg_copy_pdbs()  # 处理动态库符号

message(STATUS "wcdb source directory: ${CURRENT_PACKAGES_DIR}")

# 处理头文件
file(INSTALL "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-dbg/src/export_headers/" DESTINATION "${CURRENT_PACKAGES_DIR}/include")

# 导出 CMake 目标
file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/WCDBConfig.cmake"
        "include(CMakeFindDependencyMacro)\n"
        "find_dependency(SQLCipher)\n"
        "find_dependency(OpenSSL)\n"
        "find_dependency(zstd)\n"
        "include(\"\${CMAKE_CURRENT_LIST_DIR}/WCDBTargets.cmake\")\n"
)
vcpkg_cmake_config_fixup()

# 生成版权声明
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
