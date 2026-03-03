#pragma once
#include <QWidget>

class QGridLayout;
class QScrollArea;

class MarketsPage : public QWidget {
    Q_OBJECT
public:
    explicit MarketsPage(QWidget *parent = nullptr);

    void setDemoMarkets(); // for testing

public slots:
    void setCategory(const QString &category);

private:
    void clearCards();
    void addCard(const QString &q, const QVector<struct OutcomeView> &o, const QString &vol);

    QString m_category;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_container = nullptr;
    QGridLayout *m_grid = nullptr;
};