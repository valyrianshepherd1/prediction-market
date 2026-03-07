#include "pm/services/MarketService.h"

#include <memory>

MarketService::MarketService(MarketRepository repo) : repo_(std::move(repo)) {
}

void MarketService::listMarkets(std::optional<std::string> status,
                                int limit,
                                int offset,
                                std::function<void(std::vector<MarketRow>)> onOk,
                                std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.listMarkets(std::move(status), limit, offset, std::move(onOk), std::move(onErr));
}

void MarketService::getMarketById(const std::string &id,
                                  std::function<void(std::optional<MarketRow>)> onOk,
                                  std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.getMarketById(id, std::move(onOk), std::move(onErr));
}

void MarketService::getMarketWithOutcomesById(
    const std::string &id,
    std::function<void(std::optional<std::pair<MarketRow, std::vector<OutcomeRow> > >)> onOk,
    std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    auto onOkPtr = std::make_shared<decltype(onOk)>(std::move(onOk));
    auto onErrPtr = std::make_shared<decltype(onErr)>(std::move(onErr));

    repo_.getMarketById(
        id,
        [this, onOkPtr, onErrPtr](std::optional<MarketRow> m) mutable {
            if (!m) {
                (*onOkPtr)(std::nullopt);
                return;
            }
            const std::string marketId = m->id;

            repo_.listOutcomesByMarketId(
                marketId,
                [onOkPtr, m = std::move(*m)](std::vector<OutcomeRow> outs) mutable {
                    (*onOkPtr)(std::make_optional(std::make_pair(std::move(m), std::move(outs))));
                },
                [onErrPtr](const drogon::orm::DrogonDbException &e) mutable { (*onErrPtr)(e); });
        },
        [onErrPtr](const drogon::orm::DrogonDbException &e) mutable { (*onErrPtr)(e); });
}

void MarketService::createMarket(const std::string &question,
                                 std::function<void(MarketRow)> onOk,
                                 std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.createMarket(question, std::move(onOk), std::move(onErr));
}

void MarketService::createMarketWithOutcomes(
    const std::string &question,
    const std::vector<std::string> &outcomeTitles,
    std::function<void(MarketRow, std::vector<OutcomeRow>)> onOk,
    std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.createMarketWithOutcomes(question, outcomeTitles, std::move(onOk), std::move(onErr));
}

void MarketService::listOutcomesByMarketId(const std::string &marketId,
                                           std::function<void(std::vector<OutcomeRow>)> onOk,
                                           std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.listOutcomesByMarketId(marketId, std::move(onOk), std::move(onErr));
}

void MarketService::resolveMarket(const std::string &marketId,
                                  const std::string &winningOutcomeId,
                                  const std::string &resolvedByUserId,
                                  std::function<void(MarketRow)> onOk,
                                  std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.resolveMarket(marketId, winningOutcomeId, resolvedByUserId, std::move(onOk), std::move(onErr));
}
