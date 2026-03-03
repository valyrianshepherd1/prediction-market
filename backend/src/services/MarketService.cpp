#include <pm/services/MarketService.h>

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

void MarketService::createMarket(const std::string &question,
                                 std::function<void(MarketRow)> onOk,
                                 std::function<void(const drogon::orm::DrogonDbException &)> onErr) const {
    repo_.createMarket(question, std::move(onOk), std::move(onErr));
}
