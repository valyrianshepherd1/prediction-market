### Пока без библиотек, пусть просто собирается
cmake_minimum_required(VERSION 4.1)
project(polymarket_clone VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optionally export compile_commands.json for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(backend)
add_subdirectory(frontend)
add_subdirectory(tests)