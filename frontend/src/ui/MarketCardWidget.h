#pragma once
#include <QFrame>
#include <QString>
#include <QVector>

struct OutcomeView {
    QString name;
    int percent;
};

class QLabel;
class QProgressBar;

class MarketCardWidget : public QFrame {
    Q_OBJECT
public:
    explicit MarketCardWidget(QWidget *parent = nullptr);

    void setMarket(const QString &question,
                   const QVector<OutcomeView> &outcomes,
                   const QString &volumeText = QString());

private:
    QLabel *m_question = nullptr;
    QLabel *m_volume = nullptr;
    QWidget *m_outcomesContainer = nullptr;
};