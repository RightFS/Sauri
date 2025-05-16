set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#if(TARGET ${PROJECT_NAME}_static)
#    set_property(TARGET ${PROJECT_NAME}_static PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#    target_compile_definitions(${PROJECT_NAME}_static PRIVATE _ALLOW_KEYWORD_MACROS)
#else()
#    message(WARNING "Target ${PROJECT_NAME}_static does not exist. Cannot set MSVC_RUNTIME_LIBRARY property.")
#endif()