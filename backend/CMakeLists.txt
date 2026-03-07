cmake_minimum_required(VERSION 3.26)

project(pm_backend LANGUAGES CXX)

find_package(Drogon CONFIG REQUIRED)

# Все .cpp из backend/src/*
file(GLOB_RECURSE BACKEND_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(backend_app ${BACKEND_SOURCES})

target_compile_features(backend_app PRIVATE cxx_std_23)

# Чтобы работало: #include <pm/...>
target_include_directories(backend_app PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(backend_app PRIVATE Drogon::Drogon)

# Базовые предупреждения (без Werror)
if(MSVC)
    target_compile_options(backend_app PRIVATE /W4 /permissive-)
else()
    target_compile_options(backend_app PRIVATE -Wall -Wextra -Wpedantic)
endif()

# -------- runtime layout: кладём config рядом с бинарём --------
set(CONFIG_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")

if(EXISTS "${CONFIG_SRC_DIR}")
    add_custom_command(TARGET backend_app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CONFIG_SRC_DIR}"
            "$<TARGET_FILE_DIR:backend_app>/config"
            COMMENT "Copy backend/config -> <bin_dir>/config"
    )

    # На всякий случай создадим подкаталоги, которые ожидает конфиг
    add_custom_command(TARGET backend_app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:backend_app>/config/logs"
            COMMENT "Ensure <bin_dir>/config/logs exists"
    )

    add_custom_command(TARGET backend_app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:backend_app>/config/certs"
            COMMENT "Ensure <bin_dir>/config/certs exists"
    )
endif()