#include "MarketsPage.h"
#include "../../network/MarketApiClient.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

MarketsPage::MarketsPage(MarketApiClient *api, QWidget *parent)
    : QWidget(parent), m_api(api)
{
    auto *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("<h2>Markets</h2>", this));

    m_label = new QLabel("Markets will be listed here.", this);
    m_label->setWordWrap(true);
    layout->addWidget(m_label);

    auto *fetchBtn = new QPushButton("Fetch Markets", this);
    fetchBtn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(fetchBtn);

    auto *homeBtn = new QPushButton("Back to Home", this);
    homeBtn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(homeBtn);

    layout->addStretch(1);

    connect(fetchBtn, &QPushButton::clicked, m_api, &MarketApiClient::fetchMarkets);
    connect(homeBtn, &QPushButton::clicked, this, &MarketsPage::backHome);

    connect(m_api, &MarketApiClient::marketsReady, this, &MarketsPage::onMarkets);
    connect(m_api, &MarketApiClient::error,        this, &MarketsPage::onError);
}

void MarketsPage::onMarkets(const QString &response) {
    m_label->setText(response.isEmpty() ? "No markets returned." : response);
}

void MarketsPage::onError(const QString &message) {
    m_label->setText("Error fetching markets:\n" + message);
}