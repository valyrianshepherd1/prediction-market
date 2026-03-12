#pragma once

#include "ApiTypes.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

#include <optional>

namespace api::parse {

    [[nodiscard]] std::optional<ApiSession> sessionFromUserObject(const QJsonObject &userObject);
    [[nodiscard]] std::optional<ApiSession> sessionFromAuthObject(const QJsonObject &rootObject);

    [[nodiscard]] std::optional<ApiWallet> wallet(const QJsonObject &object);
    [[nodiscard]] std::optional<ApiTrade> trade(const QJsonObject &object);
    [[nodiscard]] std::optional<ApiOrder> order(const QJsonObject &object);
    [[nodiscard]] std::optional<ApiPortfolioPosition> portfolioPosition(const QJsonObject &object);
    [[nodiscard]] std::optional<ApiPortfolioLedgerEntry> portfolioLedgerEntry(const QJsonObject &object);
    [[nodiscard]] std::optional<ApiMarket> marketBase(const QJsonObject &object);
    [[nodiscard]] std::optional<ApiOutcome> outcome(const QJsonObject &object);
    [[nodiscard]] std::optional<ApiOrderBook> orderBook(const QJsonObject &object);

    [[nodiscard]] QVector<ApiTrade> trades(const QJsonArray &array);
    [[nodiscard]] QVector<ApiOrder> orders(const QJsonArray &array);
    [[nodiscard]] QVector<ApiPortfolioPosition> portfolioPositions(const QJsonArray &array);
    [[nodiscard]] QVector<ApiPortfolioLedgerEntry> portfolioLedgerEntries(const QJsonArray &array);
    [[nodiscard]] QVector<ApiOutcome> outcomes(const QJsonArray &array);

} // namespace api::parse
