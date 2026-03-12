#pragma once

#include "frontend/network/ApiTypes.h"

#include <QObject>
#include <QVector>

class MarketApiClient;

class PortfolioRepository : public QObject {
    Q_OBJECT
public:
    explicit PortfolioRepository(MarketApiClient *transport, QObject *parent = nullptr);

    void refreshWallet();
    void refreshPortfolio(int limit = 100, int offset = 0);
    void refreshLedger(int limit = 100, int offset = 0);
    void adminDepositToCurrentUser(qint64 amount);

    [[nodiscard]] ApiWallet wallet() const;
    [[nodiscard]] QVector<ApiPortfolioPosition> positions() const;
    [[nodiscard]] QVector<ApiPortfolioLedgerEntry> ledgerEntries() const;

signals:
    void walletReady(const ApiWallet &wallet);
    void walletError(const QString &message);

    void portfolioReady(const QVector<ApiPortfolioPosition> &positions);
    void portfolioError(const QString &message);

    void ledgerReady(const QVector<ApiPortfolioLedgerEntry> &entries);
    void ledgerError(const QString &message);

    void depositBusyChanged(bool busy);
    void depositSucceeded(const ApiWallet &wallet);
    void depositError(const QString &message);

private:
    MarketApiClient *m_transport = nullptr;
    ApiWallet m_wallet;
    QVector<ApiPortfolioPosition> m_positions;
    QVector<ApiPortfolioLedgerEntry> m_ledgerEntries;
};
