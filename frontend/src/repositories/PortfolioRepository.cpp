#include "frontend/repositories/PortfolioRepository.h"

#include "frontend/network/MarketApiClient.h"

PortfolioRepository::PortfolioRepository(MarketApiClient *transport, QObject *parent)
    : QObject(parent),
      m_transport(transport) {
    Q_ASSERT(m_transport != nullptr);

    connect(m_transport, &MarketApiClient::walletReady, this, [this](const ApiWallet &walletValue) {
        m_wallet = walletValue;
        emit walletReady(m_wallet);
    });
    connect(m_transport, &MarketApiClient::walletError, this, &PortfolioRepository::walletError);

    connect(m_transport, &MarketApiClient::portfolioReady, this, [this](const QVector<ApiPortfolioPosition> &positions) {
        m_positions = positions;
        emit portfolioReady(m_positions);
    });
    connect(m_transport, &MarketApiClient::portfolioError, this, &PortfolioRepository::portfolioError);

    connect(m_transport, &MarketApiClient::ledgerReady, this, [this](const QVector<ApiPortfolioLedgerEntry> &entries) {
        m_ledgerEntries = entries;
        emit ledgerReady(m_ledgerEntries);
    });
    connect(m_transport, &MarketApiClient::ledgerError, this, &PortfolioRepository::ledgerError);

    connect(m_transport, &MarketApiClient::depositBusyChanged, this, &PortfolioRepository::depositBusyChanged);
    connect(m_transport, &MarketApiClient::depositSucceeded, this, [this](const ApiWallet &walletValue) {
        m_wallet = walletValue;
        emit depositSucceeded(m_wallet);
        emit walletReady(m_wallet);
    });
    connect(m_transport, &MarketApiClient::depositError, this, &PortfolioRepository::depositError);
}

void PortfolioRepository::refreshWallet() {
    if (m_transport) {
        m_transport->fetchWallet();
    }
}

void PortfolioRepository::refreshPortfolio(int limit, int offset) {
    if (m_transport) {
        m_transport->fetchPortfolio(limit, offset);
    }
}

void PortfolioRepository::refreshLedger(int limit, int offset) {
    if (m_transport) {
        m_transport->fetchLedger(limit, offset);
    }
}

void PortfolioRepository::adminDepositToCurrentUser(qint64 amount) {
    if (m_transport) {
        m_transport->adminDepositToCurrentUser(amount);
    }
}

ApiWallet PortfolioRepository::wallet() const {
    return m_wallet;
}

QVector<ApiPortfolioPosition> PortfolioRepository::positions() const {
    return m_positions;
}

QVector<ApiPortfolioLedgerEntry> PortfolioRepository::ledgerEntries() const {
    return m_ledgerEntries;
}
