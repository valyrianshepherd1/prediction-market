find_package(Drogon CONFIG REQUIRED)

add_executable(backend_app
        src/main.cpp
)

target_link_libraries(backend_app PRIVATE
        Drogon::Drogon
)