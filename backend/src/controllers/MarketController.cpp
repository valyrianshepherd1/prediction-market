#include "pm/controllers/MarketController.h"

#include "pm/repositories/MarketRepository.h"
#include "pm/services/MarketService.h"
#include "pm/util/ApiError.h"
#include "pm/util/AuthGuard.h"
#include "pm/util/Expected.h"

#include <charconv>
#include <cctype>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using drogon::HttpResponse;
using drogon::HttpResponsePtr;

namespace {
using ApiError = pm::ApiError;
using ResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

template <typename T>
using Expected = pm::expected<T, ApiError>;

[[nodiscard]] Json::Value marketToJson(const MarketRow &m) {
    Json::Value j;
    j["id"] = m.id;
    j["question"] = m.question;
    j["status"] = m.status;
    j["resolved_outcome_id"] =
        m.resolved_outcome_id ? Json::Value(*m.resolved_outcome_id) : Json::Value(Json::nullValue);
    j["created_at"] = m.created_at;
    return j;
}

[[nodiscard]] Expected<int> parseInt(std::string_view s,
                                     int minV,
                                     int maxV,
                                     std::string_view fieldName) {
    int v{};
    const auto *begin = s.data();
    const auto *end = s.data() + s.size();

    auto [ptr, ec] = std::from_chars(begin, end, v);
    if (ec != std::errc{} || ptr != end) {
        return pm::unexpected(ApiError{drogon::k400BadRequest,
                                       std::string("invalid ") + std::string(fieldName)});
    }

    if (v < minV || v > maxV) {
        return pm::unexpected(ApiError{drogon::k400BadRequest,
                                       std::string(fieldName) + " must be " +
                                           std::to_string(minV) + ".." +
                                           std::to_string(maxV)});
    }

    return v;
}

struct Paging {
    int limit{};
    int offset{};
};

[[nodiscard]] Expected<Paging> parsePaging(const drogon::HttpRequestPtr &req) {
    int limit = 50;
    int offset = 0;

    if (auto s = req->getParameter("limit"); !s.empty()) {
        auto r = parseInt(s, 1, 200, "limit");
        if (!r) {
            return pm::unexpected(r.error());
        }
        limit = r.value();
    }

    if (auto s = req->getParameter("offset"); !s.empty()) {
        auto r = parseInt(s, 0, 1'000'000'000, "offset");
        if (!r) {
            return pm::unexpected(r.error());
        }
        offset = r.value();
    }

    return Paging{limit, offset};
}

constexpr std::string_view kAllowedStatuses[] = {
    "OPEN",
    "CLOSED",
    "RESOLVED",
    "ARCHIVED",
};

[[nodiscard]] bool isAllowedStatus(std::string_view s) {
    for (auto a : kAllowedStatuses) {
        if (s == a) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] Expected<std::optional<std::string>> parseStatus(const drogon::HttpRequestPtr &req) {
    const auto st = req->getParameter("status");
    if (st.empty()) {
        return std::optional<std::string>{};
    }

    if (!isAllowedStatus(st)) {
        return pm::unexpected(ApiError{drogon::k400BadRequest,
                                       "status must be OPEN|CLOSED|RESOLVED|ARCHIVED"});
    }

    return std::optional<std::string>{st};
}

[[nodiscard]] Expected<drogon::orm::DbClientPtr> getDb() {
    try {
        return drogon::app().getDbClient("default");
    } catch (const std::exception &e) {
        return pm::unexpected(ApiError{
            drogon::k500InternalServerError,
            std::string("db client not available: ") + e.what()});
    }
}

[[nodiscard]] Expected<std::string> parseQuestion(const drogon::HttpRequestPtr &req) {
    const auto json = req->getJsonObject();
    if (!json) {
        return pm::unexpected(ApiError{drogon::k400BadRequest, "expected JSON body"});
    }

    std::string q = (*json).get("question", "").asString();
    if (q.empty()) {
        q = (*json).get("title", "").asString();
    }

    if (q.size() < 3 || q.size() > 300) {
        return pm::unexpected(ApiError{drogon::k400BadRequest,
                                       "question must be 3..300 chars"});
    }

    return q;
}

[[nodiscard]] std::string trimCopy(std::string s) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };

    while (!s.empty() && isSpace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }

    return s;
}

[[nodiscard]] bool containsDuplicateTitle(const std::vector<std::string> &titles,
                                          std::string_view value) {
    for (const auto &t : titles) {
        if (t == value) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] Expected<std::vector<std::string>> parseOutcomeTitles(
    const drogon::HttpRequestPtr &req) {
    const auto json = req->getJsonObject();
    if (!json) {
        return pm::unexpected(ApiError{drogon::k400BadRequest, "expected JSON body"});
    }

    const Json::Value *arr = nullptr;
    if (json->isMember("outcomes")) {
        arr = &(*json)["outcomes"];
    } else if (json->isMember("outcome_titles")) {
        arr = &(*json)["outcome_titles"];
    }

    if (!arr) {
        return std::vector<std::string>{"YES", "NO"};
    }

    if (!arr->isArray()) {
        return pm::unexpected(ApiError{drogon::k400BadRequest,
                                       "outcomes must be an array of strings"});
    }

    std::vector<std::string> titles;
    titles.reserve(arr->size());

    for (const auto &item : *arr) {
        if (!item.isString()) {
            return pm::unexpected(ApiError{drogon::k400BadRequest,
                                           "outcomes must contain only strings"});
        }

        auto title = trimCopy(item.asString());
        if (title.size() < 1 || title.size() > 80) {
            return pm::unexpected(ApiError{drogon::k400BadRequest,
                                           "each outcome title must be 1..80 chars"});
        }
        if (containsDuplicateTitle(titles, title)) {
            return pm::unexpected(ApiError{drogon::k400BadRequest,
                                           "outcome titles must be unique"});
        }

        titles.push_back(std::move(title));
    }

    if (titles.size() < 2 || titles.size() > 10) {
        return pm::unexpected(ApiError{drogon::k400BadRequest,
                                       "outcomes count must be 2..10"});
    }

    return titles;
}

[[nodiscard]] Json::Value outcomeToJson(const OutcomeRow &o) {
    Json::Value j;
    j["id"] = o.id;
    j["market_id"] = o.market_id;
    j["title"] = o.title;
    j["outcome_index"] = o.outcome_index;
    return j;
}

[[nodiscard]] Json::Value marketWithOutcomesToJson(const MarketRow &m,
                                                   const std::vector<OutcomeRow> &outs) {
    Json::Value j = marketToJson(m);
    Json::Value arr(Json::arrayValue);
    for (const auto &o : outs) {
        arr.append(outcomeToJson(o));
    }
    j["outcomes"] = std::move(arr);
    return j;
}

struct MarketPatchRequest {
    std::optional<std::string> question;
    std::optional<std::string> status;
};

[[nodiscard]] Expected<MarketPatchRequest> parseMarketPatch(const drogon::HttpRequestPtr &req) {
    const auto json = req->getJsonObject();
    if (!json) {
        return pm::unexpected(ApiError{drogon::k400BadRequest, "expected JSON body"});
    }

    MarketPatchRequest patch{};

    if (json->isMember("question") || json->isMember("title")) {
        std::string q = (*json).get("question", "").asString();
        if (q.empty()) {
            q = (*json).get("title", "").asString();
        }

        if (q.size() < 3 || q.size() > 300) {
            return pm::unexpected(ApiError{drogon::k400BadRequest,
                                           "question must be 3..300 chars"});
        }
        patch.question = std::move(q);
    }

    if (json->isMember("status")) {
        auto st = (*json).get("status", "").asString();
        if (st != "OPEN" && st != "CLOSED") {
            return pm::unexpected(ApiError{
                drogon::k400BadRequest,
                "status must be OPEN|CLOSED (use /resolve for RESOLVED, /archive for ARCHIVED)"});
        }
        patch.status = std::move(st);
    }

    if (!patch.question && !patch.status) {
        return pm::unexpected(ApiError{drogon::k400BadRequest,
                                       "at least one of question or status is required"});
    }

    return patch;
}

}  // namespace

bool MarketController::isAdmin(const drogon::HttpRequestPtr &) {
    return false;
}

void MarketController::listMarkets(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<ResponseCallback>(std::move(cb));

    auto paging = parsePaging(req);
    if (!paging) {
        return (*cbp)(pm::jsonError(paging.error()));
    }

    auto status = parseStatus(req);
    if (!status) {
        return (*cbp)(pm::jsonError(status.error()));
    }

    auto db = getDb();
    if (!db) {
        return (*cbp)(pm::jsonError(db.error()));
    }

    const auto [limit, offset] = paging.value();
    MarketService svc{MarketRepository{db.value()}};
    svc.listMarkets(
        status.value(),
        limit,
        offset,
        [cbp](std::vector<MarketRow> rows) {
            Json::Value arr(Json::arrayValue);
            for (const auto &r : rows) {
                arr.append(marketToJson(r));
            }
            auto resp = HttpResponse::newHttpJsonResponse(arr);
            resp->setStatusCode(drogon::k200OK);
            (*cbp)(resp);
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void MarketController::getMarket(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb,
    std::string id) const {
    auto cbp = std::make_shared<ResponseCallback>(std::move(cb));

    auto db = getDb();
    if (!db) {
        return (*cbp)(pm::jsonError(db.error()));
    }

    MarketService svc{MarketRepository{db.value()}};
    svc.getMarketById(
        id,
        [cbp](std::optional<MarketRow> row) {
            if (!row) {
                return (*cbp)(pm::jsonError({drogon::k404NotFound, "market not found"}));
            }

            auto resp = HttpResponse::newHttpJsonResponse(marketToJson(*row));
            resp->setStatusCode(drogon::k200OK);
            (*cbp)(resp);
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void MarketController::createMarket(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<ResponseCallback>(std::move(cb));

    pm::auth::requireAdminUser(
        req,
        [cbp, req](pm::auth::Principal) {
            auto question = parseQuestion(req);
            if (!question) {
                return (*cbp)(pm::jsonError(question.error()));
            }

            auto outcomeTitles = parseOutcomeTitles(req);
            if (!outcomeTitles) {
                return (*cbp)(pm::jsonError(outcomeTitles.error()));
            }

            auto db = getDb();
            if (!db) {
                return (*cbp)(pm::jsonError(db.error()));
            }

            MarketService svc{MarketRepository{db.value()}};
            svc.createMarketWithOutcomes(
                question.value(),
                outcomeTitles.value(),
                [cbp](MarketRow created, std::vector<OutcomeRow> outcomes) {
                    auto resp = HttpResponse::newHttpJsonResponse(
                        marketWithOutcomesToJson(created, outcomes));
                    resp->setStatusCode(drogon::k201Created);
                    (*cbp)(resp);
                },
                [cbp](const drogon::orm::DrogonDbException &e) {
                    (*cbp)(pm::jsonError(
                        {drogon::k503ServiceUnavailable, e.base().what()}));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void MarketController::updateMarket(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb,
    std::string id) const {
    auto cbp = std::make_shared<ResponseCallback>(std::move(cb));

    pm::auth::requireAdminUser(
        req,
        [cbp, req, id = std::move(id)](pm::auth::Principal) mutable {
            auto patch = parseMarketPatch(req);
            if (!patch) {
                return (*cbp)(pm::jsonError(patch.error()));
            }

            auto db = getDb();
            if (!db) {
                return (*cbp)(pm::jsonError(db.error()));
            }

            auto svc = std::make_shared<MarketService>(MarketRepository{db.value()});
            svc->getMarketById(
                id,
                [cbp, svc, patch = patch.value()](std::optional<MarketRow> row) mutable {
                    if (!row) {
                        return (*cbp)(pm::jsonError(
                            {drogon::k404NotFound, "market not found"}));
                    }

                    if (row->status == "RESOLVED") {
                        return (*cbp)(pm::jsonError(
                            {drogon::k409Conflict, "resolved market cannot be updated"}));
                    }

                    if (row->status == "ARCHIVED") {
                        return (*cbp)(pm::jsonError(
                            {drogon::k409Conflict, "archived market cannot be updated"}));
                    }

                    svc->updateMarket(
                        row->id,
                        std::move(patch.question),
                        std::move(patch.status),
                        [cbp](std::optional<MarketRow> updated) mutable {
                            if (!updated) {
                                return (*cbp)(pm::jsonError(
                                    {drogon::k404NotFound, "market not found"}));
                            }

                            auto resp =
                                HttpResponse::newHttpJsonResponse(marketToJson(*updated));
                            resp->setStatusCode(drogon::k200OK);
                            (*cbp)(resp);
                        },
                        [cbp](const drogon::orm::DrogonDbException &e) mutable {
                            (*cbp)(pm::jsonError(
                                {drogon::k503ServiceUnavailable, e.base().what()}));
                        });
                },
                [cbp](const drogon::orm::DrogonDbException &e) mutable {
                    (*cbp)(pm::jsonError(
                        {drogon::k503ServiceUnavailable, e.base().what()}));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void MarketController::closeMarket(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb,
    std::string id) const {
    auto cbp = std::make_shared<ResponseCallback>(std::move(cb));

    pm::auth::requireAdminUser(
        req,
        [cbp, id = std::move(id)](pm::auth::Principal) mutable {
            auto db = getDb();
            if (!db) {
                return (*cbp)(pm::jsonError(db.error()));
            }

            auto svc = std::make_shared<MarketService>(MarketRepository{db.value()});
            svc->getMarketById(
                id,
                [cbp, svc](std::optional<MarketRow> row) mutable {
                    if (!row) {
                        return (*cbp)(pm::jsonError(
                            {drogon::k404NotFound, "market not found"}));
                    }

                    if (row->status == "RESOLVED") {
                        return (*cbp)(pm::jsonError(
                            {drogon::k409Conflict, "resolved market cannot be closed"}));
                    }

                    if (row->status == "ARCHIVED") {
                        return (*cbp)(pm::jsonError(
                            {drogon::k409Conflict, "archived market cannot be closed"}));
                    }

                    if (row->status == "CLOSED") {
                        auto resp = HttpResponse::newHttpJsonResponse(marketToJson(*row));
                        resp->setStatusCode(drogon::k200OK);
                        return (*cbp)(resp);
                    }

                    svc->updateMarket(
                        row->id,
                        std::nullopt,
                        std::optional<std::string>{"CLOSED"},
                        [cbp](std::optional<MarketRow> updated) mutable {
                            if (!updated) {
                                return (*cbp)(pm::jsonError(
                                    {drogon::k404NotFound, "market not found"}));
                            }

                            auto resp =
                                HttpResponse::newHttpJsonResponse(marketToJson(*updated));
                            resp->setStatusCode(drogon::k200OK);
                            (*cbp)(resp);
                        },
                        [cbp](const drogon::orm::DrogonDbException &e) mutable {
                            (*cbp)(pm::jsonError(
                                {drogon::k503ServiceUnavailable, e.base().what()}));
                        });
                },
                [cbp](const drogon::orm::DrogonDbException &e) mutable {
                    (*cbp)(pm::jsonError(
                        {drogon::k503ServiceUnavailable, e.base().what()}));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void MarketController::listOutcomes(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb,
    std::string id) const {
    auto cbp = std::make_shared<ResponseCallback>(std::move(cb));

    auto db = getDb();
    if (!db) {
        return (*cbp)(pm::jsonError(db.error()));
    }

    MarketService svc{MarketRepository{db.value()}};
    svc.listOutcomesByMarketId(
        id,
        [cbp](std::vector<OutcomeRow> outs) mutable {
            Json::Value arr(Json::arrayValue);
            for (const auto &o : outs) {
                arr.append(outcomeToJson(o));
            }

            auto resp = drogon::HttpResponse::newHttpJsonResponse(arr);
            resp->setStatusCode(drogon::k200OK);
            (*cbp)(resp);
        },
        [cbp](const drogon::orm::DrogonDbException &e) mutable {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void MarketController::resolveMarket(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb,
    std::string id) const {
    auto cbp = std::make_shared<ResponseCallback>(std::move(cb));

    pm::auth::requireAdminUser(
        req,
        [cbp, req, id = std::move(id)](pm::auth::Principal admin) mutable {
            const auto json = req->getJsonObject();
            if (!json) {
                return (*cbp)(pm::jsonError(
                    {drogon::k400BadRequest, "expected JSON body"}));
            }

            std::string winningOutcomeId = (*json).get("winning_outcome_id", "").asString();
            if (winningOutcomeId.empty()) {
                winningOutcomeId = (*json).get("outcome_id", "").asString();
            }
            if (winningOutcomeId.empty()) {
                return (*cbp)(pm::jsonError(
                    {drogon::k400BadRequest, "winning_outcome_id is required"}));
            }

            auto db = getDb();
            if (!db) {
                return (*cbp)(pm::jsonError(db.error()));
            }

            const std::string adminUserId = admin.user_id;
            auto svc = std::make_shared<MarketService>(MarketRepository{db.value()});
            svc->getMarketWithOutcomesById(
                id,
                [cbp, svc, winningOutcomeId, adminUserId](
                    std::optional<std::pair<MarketRow, std::vector<OutcomeRow>>> data) mutable {
                    if (!data) {
                        return (*cbp)(pm::jsonError(
                            {drogon::k404NotFound, "market not found"}));
                    }

                    auto &[market, outcomes] = *data;

                    if (market.status == "RESOLVED") {
                        return (*cbp)(pm::jsonError(
                            {drogon::k409Conflict, "market already resolved"}));
                    }

                    if (market.status == "ARCHIVED") {
                        return (*cbp)(pm::jsonError(
                            {drogon::k409Conflict, "archived market cannot be resolved"}));
                    }

                    bool belongsToMarket = false;
                    for (const auto &o : outcomes) {
                        if (o.id == winningOutcomeId) {
                            belongsToMarket = true;
                            break;
                        }
                    }

                    if (!belongsToMarket) {
                        return (*cbp)(pm::jsonError({
                            drogon::k400BadRequest,
                            "winning_outcome_id does not belong to market"}));
                    }

                    svc->resolveMarket(
                        market.id,
                        winningOutcomeId,
                        adminUserId,
                        [cbp](MarketRow updated) mutable {
                            auto resp = drogon::HttpResponse::newHttpJsonResponse(
                                marketToJson(updated));
                            resp->setStatusCode(drogon::k200OK);
                            (*cbp)(resp);
                        },
                        [cbp](const drogon::orm::DrogonDbException &e) mutable {
                            (*cbp)(pm::jsonError(
                                {drogon::k503ServiceUnavailable, e.base().what()}));
                        });
                },
                [cbp](const drogon::orm::DrogonDbException &e) mutable {
                    (*cbp)(pm::jsonError(
                        {drogon::k503ServiceUnavailable, e.base().what()}));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void MarketController::archiveMarket(
    const drogon::HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&cb,
    std::string id) const {
    auto cbp = std::make_shared<ResponseCallback>(std::move(cb));

    pm::auth::requireAdminUser(
        req,
        [cbp, id = std::move(id)](pm::auth::Principal) mutable {
            auto db = getDb();
            if (!db) {
                return (*cbp)(pm::jsonError(db.error()));
            }

            auto svc = std::make_shared<MarketService>(MarketRepository{db.value()});
            svc->getMarketById(
                id,
                [cbp, svc](std::optional<MarketRow> row) mutable {
                    if (!row) {
                        return (*cbp)(pm::jsonError(
                            {drogon::k404NotFound, "market not found"}));
                    }

                    if (row->status == "RESOLVED") {
                        return (*cbp)(pm::jsonError(
                            {drogon::k409Conflict, "resolved market cannot be archived"}));
                    }

                    if (row->status == "ARCHIVED") {
                        auto resp = HttpResponse::newHttpJsonResponse(marketToJson(*row));
                        resp->setStatusCode(drogon::k200OK);
                        return (*cbp)(resp);
                    }

                    svc->archiveMarket(
                        row->id,
                        [cbp](MarketRow updated) mutable {
                            auto resp = HttpResponse::newHttpJsonResponse(marketToJson(updated));
                            resp->setStatusCode(drogon::k200OK);
                            (*cbp)(resp);
                        },
                        [cbp](const drogon::orm::DrogonDbException &e) mutable {
                            (*cbp)(pm::jsonError(
                                {drogon::k503ServiceUnavailable, e.base().what()}));
                        });
                },
                [cbp](const drogon::orm::DrogonDbException &e) mutable {
                    (*cbp)(pm::jsonError(
                        {drogon::k503ServiceUnavailable, e.base().what()}));
                });
        },
        [cbp](const pm::ApiError &e) { (*cbp)(pm::jsonError(e)); },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(pm::jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void MarketController::deleteMarket(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb,
    std::string id) const {
    archiveMarket(req, std::move(cb), std::move(id));
}
