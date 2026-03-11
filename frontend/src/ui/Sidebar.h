#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;
class QButtonGroup;

class Sidebar : public QWidget {
    Q_OBJECT
public:
    explicit Sidebar(QWidget *parent = nullptr);

    void setCurrentPage(const QString &pageId);
    void setStatusText(const QString &text);

    signals:
        void pageRequested(const QString &pageId);

private:
    QPushButton *createNavButton(const QString &text, const QString &pageId);

    QVBoxLayout *m_root = nullptr;
    QLabel *m_logoLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QButtonGroup *m_group = nullptr;

    QPushButton *m_marketsButton = nullptr;
    QPushButton *m_portfolioButton = nullptr;
    QPushButton *m_ordersButton = nullptr;
    QPushButton *m_tradesButton = nullptr;
    QPushButton *m_profileButton = nullptr;
};