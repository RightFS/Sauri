# Add source files
set(SOURCES
        component_factory_impl.cpp
)

# Platform-specific source files
if (WIN32)
    list(APPEND SOURCES
            windows/registry_manager_windows.cpp
            windows/firewall_controller_windows.cpp
            windows/permission_handler_windows.cpp
            windows/process_launcher_windows.cpp
            windows/environment_variables_windows.cpp
    )
elseif (UNIX AND NOT APPLE)
    list(APPEND SOURCES
            linux/registry_manager_stub.cpp
            linux/firewall_controller_linux.cpp
            linux/permission_handler_linux.cpp
            linux/process_launcher_linux.cpp
            linux/environment_variables_linux.cpp
    )
elseif (APPLE)
    list(APPEND SOURCES
            linux/registry_manager_stub.cpp
            macos/firewall_controller_macos.cpp
            macos/permission_handler_macos.cpp
            linux/process_manager_macos.cpp
            linux/environment_manager_macos.cpp
    )
endif ()


set(SYSTEM_KIT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Add the library target
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/macos>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/linux>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/windows>
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

set_system_kit_properties(${PROJECT_NAME})