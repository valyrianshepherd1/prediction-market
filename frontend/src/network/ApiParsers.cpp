#include "../../include/frontend/network/ApiParsers.h"

namespace api::parse {

std::optional<ApiSession> sessionFromUserObject(const QJsonObject &userObject) {
    ApiSession session;
    session.userId = userObject.value(QStringLiteral("id")).toString();
    session.email = userObject.value(QStringLiteral("email")).toString();
    session.username = userObject.value(QStringLiteral("username")).toString();
    session.role = userObject.value(QStringLiteral("role")).toString();
    session.createdAt = userObject.value(QStringLiteral("created_at")).toString();

    if (session.userId.trimmed().isEmpty()) {
        return std::nullopt;
    }

    return session;
}

std::optional<ApiSession> sessionFromAuthObject(const QJsonObject &rootObject) {
    return sessionFromUserObject(rootObject.value(QStringLiteral("user")).toObject());
}

std::optional<ApiWallet> wallet(const QJsonObject &object) {
    ApiWallet walletValue;
    walletValue.userId = object.value(QStringLiteral("user_id")).toString();
    walletValue.available = object.value(QStringLiteral("available")).toVariant().toLongLong();
    walletValue.reserved = object.value(QStringLiteral("reserved")).toVariant().toLongLong();
    walletValue.updatedAt = object.value(QStringLiteral("updated_at")).toString();

    if (walletValue.userId.trimmed().isEmpty()) {
        return std::nullopt;
    }

    return walletValue;
}

std::optional<ApiTrade> trade(const QJsonObject &object) {
    ApiTrade tradeValue;
    tradeValue.id = object.value(QStringLiteral("id")).toString();
    tradeValue.outcomeId = object.value(QStringLiteral("outcome_id")).toString();
    tradeValue.makerUserId = object.value(QStringLiteral("maker_user_id")).toString();
    tradeValue.takerUserId = object.value(QStringLiteral("taker_user_id")).toString();
    tradeValue.makerOrderId = object.value(QStringLiteral("maker_order_id")).toString();
    tradeValue.takerOrderId = object.value(QStringLiteral("taker_order_id")).toString();
    tradeValue.priceBasisPoints = object.value(QStringLiteral("price_bp")).toInt();
    tradeValue.quantityMicros = object.value(QStringLiteral("qty_micros")).toVariant().toLongLong();
    tradeValue.createdAt = object.value(QStringLiteral("created_at")).toString();

    if (tradeValue.id.trimmed().isEmpty()) {
        return std::nullopt;
    }

    return tradeValue;
}

std::optional<ApiOrder> order(const QJsonObject &object) {
    ApiOrder orderValue;
    orderValue.id = object.value(QStringLiteral("id")).toString();
    orderValue.userId = object.value(QStringLiteral("user_id")).toString();
    orderValue.outcomeId = object.value(QStringLiteral("outcome_id")).toString();
    orderValue.side = object.value(QStringLiteral("side")).toString();
    orderValue.priceBasisPoints = object.value(QStringLiteral("price_bp")).toInt();
    orderValue.quantityTotalMicros =
        object.value(QStringLiteral("qty_total_micros")).toVariant().toLongLong();
    orderValue.quantityRemainingMicros =
        object.value(QStringLiteral("qty_remaining_micros")).toVariant().toLongLong();
    orderValue.status = object.value(QStringLiteral("status")).toString();
    orderValue.createdAt = object.value(QStringLiteral("created_at")).toString();
    orderValue.updatedAt = object.value(QStringLiteral("updated_at")).toString();

    if (orderValue.id.trimmed().isEmpty()) {
        return std::nullopt;
    }

    return orderValue;
}

std::optional<ApiPortfolioPosition> portfolioPosition(const QJsonObject &object) {
    ApiPortfolioPosition position;
    position.userId = object.value(QStringLiteral("user_id")).toString();
    position.outcomeId = object.value(QStringLiteral("outcome_id")).toString();
    position.marketId = object.value(QStringLiteral("market_id")).toString();
    position.marketQuestion = object.value(QStringLiteral("market_question")).toString();
    position.outcomeTitle = object.value(QStringLiteral("outcome_title")).toString();
    position.outcomeIndex = object.value(QStringLiteral("outcome_index")).toInt();
    position.sharesAvailable = object.value(QStringLiteral("shares_available")).toVariant().toLongLong();
    position.sharesReserved = object.value(QStringLiteral("shares_reserved")).toVariant().toLongLong();
    position.sharesTotal = object.value(QStringLiteral("shares_total")).toVariant().toLongLong();
    position.updatedAt = object.value(QStringLiteral("updated_at")).toString();

    if (position.outcomeId.trimmed().isEmpty()) {
        return std::nullopt;
    }

    return position;
}

std::optional<ApiPortfolioLedgerEntry> portfolioLedgerEntry(const QJsonObject &object) {
    ApiPortfolioLedgerEntry entry;
    entry.id = object.value(QStringLiteral("id")).toString();
    entry.ledgerType = object.value(QStringLiteral("ledger_type")).toString();
    entry.kind = object.value(QStringLiteral("kind")).toString();
    entry.outcomeId = object.value(QStringLiteral("outcome_id")).toString();
    entry.deltaAvailable = object.value(QStringLiteral("delta_available")).toVariant().toLongLong();
    entry.deltaReserved = object.value(QStringLiteral("delta_reserved")).toVariant().toLongLong();
    entry.refType = object.value(QStringLiteral("ref_type")).toString();
    entry.refId = object.value(QStringLiteral("ref_id")).toString();
    entry.createdAt = object.value(QStringLiteral("created_at")).toString();

    if (entry.id.trimmed().isEmpty()) {
        return std::nullopt;
    }

    return entry;
}

std::optional<ApiMarket> marketBase(const QJsonObject &object) {
    ApiMarket market;
    market.id = object.value(QStringLiteral("id")).toString();
    market.question = object.value(QStringLiteral("question")).toString();
    market.status = object.value(QStringLiteral("status")).toString();
    market.createdAt = object.value(QStringLiteral("created_at")).toString();

    if (market.id.trimmed().isEmpty()) {
        return std::nullopt;
    }

    return market;
}

std::optional<ApiOutcome> outcome(const QJsonObject &object) {
    ApiOutcome outcomeValue;
    outcomeValue.id = object.value(QStringLiteral("id")).toString();
    outcomeValue.title = object.value(QStringLiteral("title")).toString();

    if (outcomeValue.id.trimmed().isEmpty()) {
        return std::nullopt;
    }

    return outcomeValue;
}

std::optional<ApiOrderBook> orderBook(const QJsonObject &object) {
    ApiOrderBook book;

    const QJsonValue buyValue = object.value(QStringLiteral("buy"));
    const QJsonValue sellValue = object.value(QStringLiteral("sell"));

    if (!buyValue.isArray() || !sellValue.isArray()) {
        return std::nullopt;
    }

    book.buy = orders(buyValue.toArray());
    book.sell = orders(sellValue.toArray());
    return book;
}

QVector<ApiTrade> trades(const QJsonArray &array) {
    QVector<ApiTrade> out;
    out.reserve(array.size());

    for (const QJsonValue &value : array) {
        if (const auto parsed = trade(value.toObject()); parsed.has_value()) {
            out.push_back(*parsed);
        }
    }

    return out;
}

QVector<ApiOrder> orders(const QJsonArray &array) {
    QVector<ApiOrder> out;
    out.reserve(array.size());

    for (const QJsonValue &value : array) {
        if (const auto parsed = order(value.toObject()); parsed.has_value()) {
            out.push_back(*parsed);
        }
    }

    return out;
}

QVector<ApiPortfolioPosition> portfolioPositions(const QJsonArray &array) {
    QVector<ApiPortfolioPosition> out;
    out.reserve(array.size());

    for (const QJsonValue &value : array) {
        if (const auto parsed = portfolioPosition(value.toObject()); parsed.has_value()) {
            out.push_back(*parsed);
        }
    }

    return out;
}

QVector<ApiPortfolioLedgerEntry> portfolioLedgerEntries(const QJsonArray &array) {
    QVector<ApiPortfolioLedgerEntry> out;
    out.reserve(array.size());

    for (const QJsonValue &value : array) {
        if (const auto parsed = portfolioLedgerEntry(value.toObject()); parsed.has_value()) {
            out.push_back(*parsed);
        }
    }

    return out;
}

QVector<ApiOutcome> outcomes(const QJsonArray &array) {
    QVector<ApiOutcome> out;
    out.reserve(array.size());

    for (const QJsonValue &value : array) {
        if (const auto parsed = outcome(value.toObject()); parsed.has_value()) {
            out.push_back(*parsed);
        }
    }

    return out;
}

} // namespace api::parse
