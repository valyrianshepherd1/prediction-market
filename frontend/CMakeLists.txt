cmake_minimum_required(VERSION 3.26)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(frontend_app
        src/main.cpp

        include/frontend/app/MarketWindow.h
        src/app/MarketWindow.cpp

        include/frontend/network/MarketApiClient.h
        src/network/MarketApiClient.cpp

        include/frontend/ui/HeaderBar.h
        src/ui/HeaderBar.cpp
        include/frontend/ui/Sidebar.h
        src/ui/Sidebar.cpp
        include/frontend/ui/MarketCardWidget.h
        src/ui/MarketCardWidget.cpp

        include/frontend/ui/pages/HomePage.h
        src/ui/pages/HomePage.cpp
        include/frontend/ui/pages/MarketsPage.h
        src/ui/pages/MarketsPage.cpp
        include/frontend/ui/pages/ProfilePage.h
        src/ui/pages/ProfilePage.cpp
        include/frontend/ui/pages/TradesPage.h
        src/ui/pages/TradesPage.cpp
        include/frontend/ui/pages/PortfolioPage.h
        src/ui/pages/PortfolioPage.cpp
        include/frontend/ui/pages/OrdersPage.h
        src/ui/pages/OrdersPage.cpp
        include/frontend/ui/pages/AuthRequiredPage.h
        src/ui/pages/AuthRequiredPage.cpp
        include/frontend/ui/pages/MarketDetailsPage.h
        src/ui/pages/MarketDetailsPage.cpp

        include/frontend/ui/dialogs/AuthDialog.h
        src/ui/dialogs/AuthDialog.cpp
        include/frontend/ui/pages/OrdersPage.h
        src/ui/pages/OrdersPage.cpp
        include/frontend/ui/pages/PortfolioPage.h
        src/ui/pages/PortfolioPage.cpp
)

target_link_libraries(frontend_app PRIVATE Qt6::Widgets Qt6::Network)