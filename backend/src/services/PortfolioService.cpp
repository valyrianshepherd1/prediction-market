#include "pm/services/PortfolioService.h"

PortfolioService::PortfolioService(PortfolioRepository repo) : repo_(std::move(repo)) {}

void PortfolioService::listPositions(
    const std::string &userId,
    int limit,
    int offset,
    std::function<void(std::vector<PortfolioPositionRow>)> onOk,
    std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.listPositionsForUser(userId, limit, offset, std::move(onOk), std::move(onErr));
}

void PortfolioService::listLedger(
    const std::string &userId,
    int limit,
    int offset,
    std::function<void(std::vector<PortfolioLedgerEntryRow>)> onOk,
    std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.listLedgerForUser(userId, limit, offset, std::move(onOk), std::move(onErr));
}
