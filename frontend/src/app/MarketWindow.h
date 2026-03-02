#pragma once
#include <QMainWindow>

class QStackedWidget;
class HeaderBar;
class HomePage;
class MarketsPage;
class MarketApiClient;

class MarketWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MarketWindow(QWidget *parent = nullptr);

private slots:
    void showHome();
    void showMarkets();
    void openProfile();

private:
    QStackedWidget *m_stack = nullptr;
    HeaderBar *m_header = nullptr;
    HomePage *m_home = nullptr;
    MarketsPage *m_markets = nullptr;

    MarketApiClient *m_api = nullptr;
};