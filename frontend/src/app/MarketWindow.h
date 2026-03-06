#pragma once
#include <QMainWindow>

class HeaderBar;
class Sidebar;
class MarketsPage;
class QStackedWidget;
class ProfilePage;

class MarketWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MarketWindow(QWidget *parent = nullptr);

private slots:
    void onCategoryChanged(const QString &cat);
    void openProfile();
    void showMarkets();

private:
    HeaderBar *m_header = nullptr;
    Sidebar *m_sidebar = nullptr;
    MarketsPage *m_markets = nullptr;

    QStackedWidget *m_stack = nullptr;
    ProfilePage *m_profile = nullptr;

    QString m_currentCategory = "Trending";
};