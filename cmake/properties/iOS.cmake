find_library(UIKit UIKit)
# 链接库
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${UIKit}
)

# 合并多个符号表
set(EXPORTED_SYMBOLS_SDK "${LEIGOD_IM_SDK_SRC_DIR}/darwin/exported_symbols.list")
set(EXPORTED_SYMBOLS_CONFIG "${LEIGOD_IM_SDK_SRC_DIR}/darwin/configuration/exported_symbols.list")

# 定义合并后的符号列表文件路径
set(MERGED_EXPORTED_SYMBOLS_FILE "${LEIGOD_IM_SDK_OUT_DIR}/merged_symbols.list")

# 使用自定义命令合并两个符号列表文件
add_custom_command(
        OUTPUT "${MERGED_EXPORTED_SYMBOLS_FILE}"
        COMMAND ${CMAKE_COMMAND} -E cat "${EXPORTED_SYMBOLS_SDK}" "${EXPORTED_SYMBOLS_CONFIG}" > "${MERGED_EXPORTED_SYMBOLS_FILE}"
        DEPENDS "${EXPORTED_SYMBOLS_SDK}" "${EXPORTED_SYMBOLS_CONFIG}"
)

# 添加一个自定义目标以确保合并操作在编译前完成
add_custom_target(merge_symbols ALL DEPENDS "${MERGED_EXPORTED_SYMBOLS_FILE}")

set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "-Wl,-exported_symbols_list, ${MERGED_EXPORTED_SYMBOLS_FILE}"
)

add_dependencies(${PROJECT_NAME} merge_symbols)

set_target_properties(${PROJECT_NAME} PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION A
    MACOSX_FRAMEWORK_IDENTIFIER com.nn.imsdk
    MACOSX_FRAMEWORK_INFO_PLIST ${LEIGOD_IM_SDK_SRC_DIR}/darwin/Info.plist
    # "current version" in semantic format in Mach-O binary file
    VERSION 1.0.0
    # "compatibility version" in semantic format in Mach-O binary file
    SOVERSION 1.0.0
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying module.modulemap"
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/Modules/module.modulemap
    ${LEIGOD_IM_SDK_OUT_DIR}/${PROJECT_NAME}.framework/Modules/module.modulemap
    COMMAND ${CMAKE_COMMAND} -E echo "module.modulemap copied"
)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "clean dSYM file"
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${LEIGOD_IM_SDK_OUT_DIR}/${PROJECT_NAME}.framework/${PROJECT_NAME} -o ${LEIGOD_IM_SDK_OUT_DIR}/${PROJECT_NAME}.dSYM
        COMMAND ${CMAKE_COMMAND} -E echo "clean dSYM file completed"
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Export symbols to dSYM file"
        COMMAND dsymutil ${LEIGOD_IM_SDK_OUT_DIR}/${PROJECT_NAME}.framework/${PROJECT_NAME} -o ${LEIGOD_IM_SDK_OUT_DIR}/${PROJECT_NAME}.dSYM
        COMMAND ${CMAKE_COMMAND} -E echo "Export symbols completed"
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Stripping symbols from the binary"
        COMMAND strip -x ${LEIGOD_IM_SDK_OUT_DIR}/${PROJECT_NAME}.framework/${PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E echo "Stripping completed"
    )
endif ()