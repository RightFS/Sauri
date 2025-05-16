cmake_minimum_required(VERSION 3.16)
project(ExampleProject VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add source files
add_executable(system_kit_example example.cpp)

# Link against the main project library if needed
target_link_libraries(system_kit_example PRIVATE leigod_systemkit)

target_compile_options(system_kit_example
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /utf-8 /wd4100>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror -pedantic>
)