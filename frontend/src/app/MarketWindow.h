#pragma once
#include <QMainWindow>

class HeaderBar;
class Sidebar;
class MarketsPage;

class MarketWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MarketWindow(QWidget *parent = nullptr);

private slots:
    void onCategoryChanged(const QString &cat);
    void openProfile();

private:
    HeaderBar *m_header = nullptr;
    Sidebar *m_sidebar = nullptr;
    MarketsPage *m_markets = nullptr;
};