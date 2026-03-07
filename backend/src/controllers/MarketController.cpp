#include <pm/controllers/MarketController.h>

#include <charconv>
#include <cctype>
#include <cstdlib>
#include <drogon/drogon.h>
#include <json/json.h>
#include <memory>
#include <optional>
#include <pm/repositories/MarketRepository.h>
#include <pm/services/MarketService.h>
#include <pm/util/Expected.h>
#include <string>
#include <string_view>
#include <utility>

using drogon::HttpResponse;
using drogon::HttpResponsePtr;

namespace {
    struct ApiError {
        drogon::HttpStatusCode code{};
        std::string message;
    };

    template<class T>
    using Expected = pm::expected<T, ApiError>;

    [[nodiscard]] HttpResponsePtr jsonError(const ApiError &e) {
        Json::Value j;
        j["error"] = e.message;
        auto resp = HttpResponse::newHttpJsonResponse(j);
        resp->setStatusCode(e.code);
        return resp;
    }

    [[nodiscard]] Json::Value marketToJson(const MarketRow &m) {
        Json::Value j;
        j["id"] = m.id;
        j["question"] = m.question;
        j["status"] = m.status;
        j["resolved_outcome_id"] = m.resolved_outcome_id
                                       ? Json::Value(*m.resolved_outcome_id)
                                       : Json::Value(Json::nullValue);
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
            return pm::unexpected<ApiError>({
                drogon::k400BadRequest,
                std::string("invalid ") + std::string(fieldName)
            });
        }
        if (v < minV || v > maxV) {
            return pm::unexpected<ApiError>(
                {
                    drogon::k400BadRequest,
                    std::string(fieldName) + " must be " + std::to_string(minV) + ".." +
                    std::to_string(maxV)
                });
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
            if (!r) return pm::unexpected<ApiError>(r.error());
            limit = r.value();
        }
        if (auto s = req->getParameter("offset"); !s.empty()) {
            auto r = parseInt(s, 0, 1'000'000'000, "offset");
            if (!r) return pm::unexpected<ApiError>(r.error());
            offset = r.value();
        }
        return Paging{limit, offset};
    }

    constexpr std::string_view kAllowedStatuses[] = {"OPEN", "CLOSED", "RESOLVED"};

    [[nodiscard]] bool isAllowedStatus(std::string_view s) {
        for (auto a: kAllowedStatuses) {
            if (s == a) return true;
        }
        return false;
    }

    [[nodiscard]] Expected<std::optional<std::string> > parseStatus(
        const drogon::HttpRequestPtr &req) {
        const auto st = req->getParameter("status");
        if (st.empty()) return std::optional<std::string>{};

        if (!isAllowedStatus(st)) {
            return pm::unexpected<ApiError>(
                {drogon::k400BadRequest, "status must be OPEN|CLOSED|RESOLVED"});
        }
        return std::optional<std::string>{st};
    }

    [[nodiscard]] Expected<drogon::orm::DbClientPtr> getDb() {
        try {
            return drogon::app().getDbClient("default");
        } catch (const std::exception &e) {
            return pm::unexpected<ApiError>(
                {
                    drogon::k500InternalServerError,
                    std::string("db client not available: ") + e.what()
                });
        }
    }

    [[nodiscard]] Expected<void> requireAdmin(const drogon::HttpRequestPtr &req) {
        const char *expected = std::getenv("PM_ADMIN_TOKEN");
        if (!expected || std::string_view(expected).empty()) {
            return pm::unexpected<ApiError>(
                {drogon::k401Unauthorized, "admin token is not configured"});
        }
        const auto token = req->getHeader("X-Admin-Token");
        if (token.empty() || token != expected) {
            return pm::unexpected<ApiError>(
                {drogon::k401Unauthorized, "admin token missing or invalid"});
        }
        return Expected<void>{};
    }

    [[nodiscard]] Expected<std::string> parseQuestion(const drogon::HttpRequestPtr &req) {
        const auto json = req->getJsonObject();
        if (!json) {
            return pm::unexpected<ApiError>(
                {drogon::k400BadRequest, "expected JSON body"});
        }

        // принимаем question, но поддержим title как алиас
        std::string q = (*json).get("question", "").asString();
        if (q.empty())
            q = (*json).get("title", "").asString();

        if (q.size() < 3 || q.size() > 300) {
            return pm::unexpected<ApiError>(
                {drogon::k400BadRequest, "question must be 3..300 chars"});
        }
        return q;
    }

    [[nodiscard]] std::string trimCopy(std::string s) {
        auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };

        while (!s.empty() && isSpace(static_cast<unsigned char>(s.front())))
            s.erase(s.begin());
        while (!s.empty() && isSpace(static_cast<unsigned char>(s.back())))
            s.pop_back();

        return s;
    }

    [[nodiscard]] bool containsDuplicateTitle(const std::vector<std::string> &titles,
                                              std::string_view value) {
        for (const auto &t : titles) {
            if (t == value)
                return true;
        }
        return false;
    }

    [[nodiscard]] Expected<std::vector<std::string>> parseOutcomeTitles(
        const drogon::HttpRequestPtr &req) {
        const auto json = req->getJsonObject();
        if (!json) {
            return pm::unexpected<ApiError>(
                {drogon::k400BadRequest, "expected JSON body"});
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
            return pm::unexpected<ApiError>(
                {drogon::k400BadRequest, "outcomes must be an array of strings"});
        }

        std::vector<std::string> titles;
        titles.reserve(arr->size());

        for (const auto &item : *arr) {
            if (!item.isString()) {
                return pm::unexpected<ApiError>(
                    {drogon::k400BadRequest, "outcomes must contain only strings"});
            }

            auto title = trimCopy(item.asString());
            if (title.size() < 1 || title.size() > 80) {
                return pm::unexpected<ApiError>(
                    {drogon::k400BadRequest,
                     "each outcome title must be 1..80 chars"});
            }
            if (containsDuplicateTitle(titles, title)) {
                return pm::unexpected<ApiError>(
                    {drogon::k400BadRequest, "outcome titles must be unique"});
            }

            titles.push_back(std::move(title));
        }

        if (titles.size() < 2 || titles.size() > 10) {
            return pm::unexpected<ApiError>(
                {drogon::k400BadRequest, "outcomes count must be 2..10"});
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
        for (const auto &o : outs)
            arr.append(outcomeToJson(o));
        j["outcomes"] = std::move(arr);
        return j;
    }
} // namespace

bool MarketController::isAdmin(const drogon::HttpRequestPtr &req) {
    return static_cast<bool>(requireAdmin(req));
}

void MarketController::listMarkets(const drogon::HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)> >(std::move(cb));

    auto paging = parsePaging(req);
    if (!paging)
        return (*cbp)(jsonError(paging.error()));

    auto status = parseStatus(req);
    if (!status)
        return (*cbp)(jsonError(status.error()));

    auto db = getDb();
    if (!db)
        return (*cbp)(jsonError(db.error()));

    const auto [limit, offset] = paging.value();
    MarketService svc{MarketRepository{db.value()}};

    svc.listMarkets(
        status.value(), limit, offset,
        [cbp](std::vector<MarketRow> rows) {
            Json::Value arr(Json::arrayValue);
            for (const auto &r: rows)
                arr.append(marketToJson(r));
            auto resp = HttpResponse::newHttpJsonResponse(arr);
            resp->setStatusCode(drogon::k200OK);
            (*cbp)(resp);
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void MarketController::getMarket(const drogon::HttpRequestPtr &,
                                 std::function<void(const HttpResponsePtr &)> &&cb,
                                 std::string id) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)> >(std::move(cb));

    auto db = getDb();
    if (!db)
        return (*cbp)(jsonError(db.error()));

    MarketService svc{MarketRepository{db.value()}};
    svc.getMarketById(
        id,
        [cbp](std::optional<MarketRow> row) {
            if (!row)
                return (*cbp)(jsonError({drogon::k404NotFound, "market not found"}));
            auto resp = HttpResponse::newHttpJsonResponse(marketToJson(*row));
            resp->setStatusCode(drogon::k200OK);
            (*cbp)(resp);
        },
        [cbp](const drogon::orm::DrogonDbException &e) {
            (*cbp)(jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void MarketController::createMarket(const drogon::HttpRequestPtr &req,
                                    std::function<void(const HttpResponsePtr &)> &&cb) const {
    auto cbp = std::make_shared<std::function<void(const HttpResponsePtr &)> >(std::move(cb));

    if (auto adm = requireAdmin(req); !adm)
        return (*cbp)(jsonError(adm.error()));

    auto question = parseQuestion(req);
    if (!question)
        return (*cbp)(jsonError(question.error()));

    auto outcomeTitles = parseOutcomeTitles(req);
    if (!outcomeTitles)
        return (*cbp)(jsonError(outcomeTitles.error()));

    auto db = getDb();
    if (!db)
        return (*cbp)(jsonError(db.error()));

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
            (*cbp)(jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

void MarketController::listOutcomes(const drogon::HttpRequestPtr&,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                   std::string id) const {
    auto cbp = std::make_shared<std::function<void(const drogon::HttpResponsePtr&)>>(std::move(cb));

    drogon::orm::DbClientPtr db;
    try {
        db = drogon::app().getDbClient("default");
    } catch (const std::exception& e) {
        Json::Value j;
        j["error"] = std::string("db client not available: ") + e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(j);
        resp->setStatusCode(drogon::k500InternalServerError);
        return (*cbp)(resp);
    }

    MarketService svc{MarketRepository{db}};
    svc.listOutcomesByMarketId(
        id,
        [cbp](std::vector<OutcomeRow> outs) mutable {
          Json::Value arr(Json::arrayValue);
          for (const auto& o : outs) {
            Json::Value j;
            j["id"] = o.id;
            j["market_id"] = o.market_id;
            j["title"] = o.title;
            j["outcome_index"] = o.outcome_index;
            arr.append(j);
          }
          auto resp = drogon::HttpResponse::newHttpJsonResponse(arr);
          resp->setStatusCode(drogon::k200OK);
          (*cbp)(resp);
        },
        [cbp](const drogon::orm::DrogonDbException& e) mutable {
          Json::Value j;
          j["error"] = e.base().what();
          auto resp = drogon::HttpResponse::newHttpJsonResponse(j);
          resp->setStatusCode(drogon::k503ServiceUnavailable);
          (*cbp)(resp);
        });
}

void MarketController::resolveMarket(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb,
    std::string id) const {
    auto cbp = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(std::move(cb));

    // 1) Проверяем admin token
    if (auto adm = requireAdmin(req); !adm) {
        return (*cbp)(jsonError(adm.error()));
    }

    // 2) Читаем JSON body
    const auto json = req->getJsonObject();
    if (!json) {
        return (*cbp)(jsonError({drogon::k400BadRequest, "expected JSON body"}));
    }

    // 3) Достаём winning_outcome_id
    std::string winningOutcomeId = (*json).get("winning_outcome_id", "").asString();
    if (winningOutcomeId.empty()) {
        winningOutcomeId = (*json).get("outcome_id", "").asString();
    }
    if (winningOutcomeId.empty()) {
        return (*cbp)(jsonError(
            {drogon::k400BadRequest, "winning_outcome_id is required"}));
    }

    // 4) Берём admin user id из env
    const char *adminUserIdEnv = std::getenv("PM_ADMIN_USER_ID");
    if (!adminUserIdEnv || std::string_view(adminUserIdEnv).empty()) {
        return (*cbp)(jsonError(
            {drogon::k500InternalServerError, "PM_ADMIN_USER_ID is not configured"}));
    }
    const std::string adminUserId = adminUserIdEnv;

    // 5) Получаем db client
    auto db = getDb();
    if (!db) {
        return (*cbp)(jsonError(db.error()));
    }

    // 6) Сервис через shared_ptr
    auto svc = std::make_shared<MarketService>(MarketRepository{db.value()});

    // 7) Сначала читаем market + outcomes
    svc->getMarketWithOutcomesById(
        id,
        [cbp, svc, winningOutcomeId, adminUserId](
            std::optional<std::pair<MarketRow, std::vector<OutcomeRow>>> data) mutable {
            if (!data) {
                return (*cbp)(jsonError({drogon::k404NotFound, "market not found"}));
            }

            auto &[market, outcomes] = *data;

            // 8) Нельзя резолвить уже RESOLVED market
            if (market.status == "RESOLVED") {
                return (*cbp)(jsonError(
                    {drogon::k409Conflict, "market already resolved"}));
            }

            // 9) Проверяем, что winningOutcomeId принадлежит этому market
            bool belongsToMarket = false;
            for (const auto &o : outcomes) {
                if (o.id == winningOutcomeId) {
                    belongsToMarket = true;
                    break;
                }
            }

            if (!belongsToMarket) {
                return (*cbp)(jsonError(
                    {drogon::k400BadRequest,
                     "winning_outcome_id does not belong to market"}));
            }

            // 10) Теперь применяем resolve через service/repository
            svc->resolveMarket(
                market.id,
                winningOutcomeId,
                adminUserId,
                [cbp](MarketRow updated) mutable {
                    auto resp =
                        drogon::HttpResponse::newHttpJsonResponse(marketToJson(updated));
                    resp->setStatusCode(drogon::k200OK);
                    (*cbp)(resp);
                },
                [cbp](const drogon::orm::DrogonDbException &e) mutable {
                    (*cbp)(jsonError(
                        {drogon::k503ServiceUnavailable, e.base().what()}));
                });
        },
        [cbp](const drogon::orm::DrogonDbException &e) mutable {
            (*cbp)(jsonError({drogon::k503ServiceUnavailable, e.base().what()}));
        });
}

