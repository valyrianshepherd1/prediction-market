#include "MarketCardWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>

MarketCardWidget::MarketCardWidget(QWidget *parent) : QFrame(parent) {
    setObjectName("MarketCard");
    setFrameShape(QFrame::NoFrame);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    m_question = new QLabel(this);
    m_question->setWordWrap(true);
    m_question->setObjectName("MarketQuestion");

    m_volume = new QLabel(this);
    m_volume->setObjectName("MarketVolume");

    m_outcomesContainer = new QWidget(this);
    auto *outLayout = new QVBoxLayout(m_outcomesContainer);
    outLayout->setContentsMargins(0, 0, 0, 0);
    outLayout->setSpacing(8);

    root->addWidget(m_question);
    root->addWidget(m_outcomesContainer);
    root->addWidget(m_volume);
}

void MarketCardWidget::setMarket(const QString &question,
                                 const QVector<OutcomeView> &outcomes,
                                 const QString &volumeText)
{
    m_question->setText(question);
    m_volume->setText(volumeText);

    auto *outLayout = qobject_cast<QVBoxLayout*>(m_outcomesContainer->layout());
    while (outLayout->count()) {
        auto *item = outLayout->takeAt(0);
        delete item->widget();
        delete item;
    }

    for (const auto &o : outcomes) {
        QWidget *row = new QWidget(m_outcomesContainer);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(10);

        auto *name = new QLabel(o.name, row);
        name->setObjectName("OutcomeName");

        auto *bar = new QProgressBar(row);
        bar->setRange(0, 100);
        bar->setValue(o.percent);
        bar->setTextVisible(false);
        bar->setFixedHeight(10);
        bar->setObjectName("OutcomeBar");

        auto *pct = new QLabel(QString::number(o.percent) + "%", row);
        pct->setObjectName("OutcomePct");
        pct->setFixedWidth(40);
        pct->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        h->addWidget(name, 1);
        h->addWidget(bar, 2);
        h->addWidget(pct);

        outLayout->addWidget(row);
    }
}