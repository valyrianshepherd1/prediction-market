cmake_minimum_required(VERSION 3.26)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(frontend_app
        src/main.cpp
)

target_link_libraries(frontend_app PRIVATE Qt6::Widgets Qt6::Network)