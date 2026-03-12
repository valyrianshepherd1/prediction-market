#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

struct ApiOutcome {
    QString id;
    QString title;
    int pricePercent = 50;
    bool hasLivePrice = false;
};

struct ApiMarket {
    QString id;
    QString question;
    QString status;
    QString createdAt;
    QVector<ApiOutcome> outcomes;
};

struct ApiWallet {
    QString userId;
    qint64 available = 0;
    qint64 reserved = 0;
    QString updatedAt;
};

struct ApiTrade {
    QString id;
    QString outcomeId;
    QString makerUserId;
    QString takerUserId;
    QString makerOrderId;
    QString takerOrderId;
    int priceBasisPoints = 0;
    qint64 quantityMicros = 0;
    QString createdAt;
};

struct ApiOrder {
    QString id;
    QString userId;
    QString outcomeId;
    QString side;
    int priceBasisPoints = 0;
    qint64 quantityTotalMicros = 0;
    qint64 quantityRemainingMicros = 0;
    QString status;
    QString createdAt;
    QString updatedAt;
};

struct ApiOrderBook {
    QVector<ApiOrder> buy;
    QVector<ApiOrder> sell;
};

struct ApiPortfolioPosition {
    QString userId;
    QString outcomeId;
    QString marketId;
    QString marketQuestion;
    QString outcomeTitle;
    int outcomeIndex = 0;
    qint64 sharesAvailable = 0;
    qint64 sharesReserved = 0;
    qint64 sharesTotal = 0;
    QString updatedAt;
};

struct ApiPortfolioLedgerEntry {
    QString id;
    QString ledgerType;
    QString kind;
    QString outcomeId;
    qint64 deltaAvailable = 0;
    qint64 deltaReserved = 0;
    QString refType;
    QString refId;
    QString createdAt;
};

struct ApiSession {
    QString userId;
    QString email;
    QString username;
    QString role;
    QString createdAt;
};

struct ApiMarketDetailsSnapshot {
    ApiMarket market;
    QString selectedOutcomeId;
    ApiOrderBook orderBook;
    QVector<ApiTrade> recentTrades;
};

Q_DECLARE_METATYPE(ApiOutcome)
Q_DECLARE_METATYPE(ApiMarket)
Q_DECLARE_METATYPE(ApiWallet)
Q_DECLARE_METATYPE(ApiTrade)
Q_DECLARE_METATYPE(ApiOrder)
Q_DECLARE_METATYPE(ApiOrderBook)
Q_DECLARE_METATYPE(ApiPortfolioPosition)
Q_DECLARE_METATYPE(ApiPortfolioLedgerEntry)
Q_DECLARE_METATYPE(ApiSession)
Q_DECLARE_METATYPE(ApiMarketDetailsSnapshot)
Q_DECLARE_METATYPE(QVector<ApiMarket>)
Q_DECLARE_METATYPE(QVector<ApiTrade>)
Q_DECLARE_METATYPE(QVector<ApiOrder>)
Q_DECLARE_METATYPE(QVector<ApiPortfolioPosition>)
Q_DECLARE_METATYPE(QVector<ApiPortfolioLedgerEntry>)
