cmake_minimum_required(VERSION 3.26)
project(prediction_market VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optionally export compile_commands.json for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable in-source build
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Use -B build/")
endif()

add_subdirectory(backend)
add_subdirectory(frontend)
add_subdirectory(tests)