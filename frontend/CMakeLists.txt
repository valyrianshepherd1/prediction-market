cmake_minimum_required(VERSION 3.26)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(frontend_app
        src/main.cpp

        src/app/MarketWindow.h
        src/app/MarketWindow.cpp

        src/network/MarketApiClient.h
        src/network/MarketApiClient.cpp

        src/ui/HeaderBar.h
        src/ui/HeaderBar.cpp
        src/ui/Sidebar.h
        src/ui/Sidebar.cpp
        src/ui/MarketCardWidget.h
        src/ui/MarketCardWidget.cpp

        src/ui/pages/HomePage.h
        src/ui/pages/HomePage.cpp
        src/ui/pages/MarketsPage.h
        src/ui/pages/MarketsPage.cpp
        src/ui/pages/ProfilePage.h
        src/ui/pages/ProfilePage.cpp
        src/ui/pages/TradesPage.h
        src/ui/pages/TradesPage.cpp
        src/ui/pages/AuthRequiredPage.h
        src/ui/pages/AuthRequiredPage.cpp

        src/ui/dialogs/AuthDialog.h
        src/ui/dialogs/AuthDialog.cpp
        src/ui/pages/AuthRequiredPage.h
        src/ui/pages/AuthRequiredPage.cpp
        src/ui/dialogs/AuthDialog.h
        src/ui/dialogs/AuthDialog.cpp
        src/ui/pages/MarketDetailsPage.h
        src/ui/pages/MarketDetailsPage.cpp
)

target_link_libraries(frontend_app PRIVATE Qt6::Widgets Qt6::Network)