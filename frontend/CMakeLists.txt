cmake_minimum_required(VERSION 3.26)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network WebSockets)

file(GLOB_RECURSE FRONTEND_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE FRONTEND_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

add_executable(frontend_app
        ${FRONTEND_SOURCES}
        ${FRONTEND_HEADERS}
)

target_include_directories(frontend_app PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(frontend_app PRIVATE
        Qt6::Widgets
        Qt6::Network
        Qt6::WebSockets
)
