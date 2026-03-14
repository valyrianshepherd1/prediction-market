<<<<<<< Updated upstream
enable_testing()
add_test(NAME dummy_test COMMAND backend_app)
=======
cmake_minimum_required(VERSION 3.26)

project(pm_tests LANGUAGES CXX)

enable_testing()

set(CMAKE_AUTOMOC ON)

find_package(Drogon CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets)

# собирает короткий Qt-тест с общими настройками.
function(add_pm_qt_test target source_file)
    add_executable(${target} ${source_file})

    target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_SOURCE_DIR}/backend/include
    )

    target_compile_definitions(${target} PRIVATE
            PM_REPO_ROOT="${CMAKE_SOURCE_DIR}"
            PM_BUILD_ROOT="${CMAKE_BINARY_DIR}"
    )

    target_link_libraries(${target} PRIVATE
            Qt6::Core
            Qt6::Network
            Qt6::WebSockets
    )
endfunction()

# собирает минимальный unit-тест для backend-утилит.
add_executable(pm_backend_utils_tests unit/test_backend_utils.cpp)

target_include_directories(pm_backend_utils_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/backend/include
)

target_link_libraries(pm_backend_utils_tests PRIVATE
        Drogon::Drogon
)

add_test(NAME pm_backend_utils_tests COMMAND pm_backend_utils_tests)

# собирает минимальные HTTP-сценарии.
add_pm_qt_test(pm_api_scenarios_tests integration/test_api_scenarios.cpp)

# запускает только базовый сценарий без лишних аргументов.
add_test(NAME pm_api_smoke COMMAND pm_api_scenarios_tests)

# запускает расширенный сценарий с --deep.
add_test(NAME pm_api_deep COMMAND pm_api_scenarios_tests --deep)

# собирает минимальный WebSocket-сценарий.
add_pm_qt_test(pm_ws_scenarios_tests integration/test_ws_scenarios.cpp)

add_test(NAME pm_ws_scenarios COMMAND pm_ws_scenarios_tests)
>>>>>>> Stashed changes
