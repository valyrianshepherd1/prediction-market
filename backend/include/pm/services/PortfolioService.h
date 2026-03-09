#pragma once

#include "pm/repositories/PortfolioRepository.h"

class PortfolioService {
public:
    explicit PortfolioService(PortfolioRepository repo);

    void listPositions(const std::string &userId,
                       int limit,
                       int offset,
                       std::function<void(std::vector<PortfolioPositionRow>)> onOk,
                       std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

    void listLedger(const std::string &userId,
                    int limit,
                    int offset,
                    std::function<void(std::vector<PortfolioLedgerEntryRow>)> onOk,
                    std::function<void(const drogon::orm::DrogonDbException &)> onErr) const;

private:
    PortfolioRepository repo_;
};
