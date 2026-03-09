#include "pm/repositories/AuthRepository.h"

#include <memory>
#include <utility>

using drogon::orm::DbClientPtr;
using drogon::orm::DrogonDbException;
using drogon::orm::Result;
using TransactionPtr = std::shared_ptr<drogon::orm::Transaction>;

namespace {
AuthUserRow rowToUser(const Result &r, std::size_t i = 0) {
    AuthUserRow u;
    const auto &row = r[i];
    u.id = row["id"].as<std::string>();
    u.email = row["email"].as<std::string>();
    u.username = row["username"].as<std::string>();
    u.role = row["role"].as<std::string>();
    u.created_at = row["created_at"].as<std::string>();
    return u;
}

AuthSessionRow rowToSession(const Result &r, std::size_t i = 0) {
    AuthSessionRow s;
    const auto &row = r[i];
    s.session_id = row["session_id"].as<std::string>();
    s.user.id = row["user_id"].as<std::string>();
    if (row["email"].isNull()) {
        s.user.email.clear();
    } else {
        s.user.email = row["email"].as<std::string>();
    }
    if (row["username"].isNull()) {
        s.user.username.clear();
    } else {
        s.user.username = row["username"].as<std::string>();
    }
    if (row["role"].isNull()) {
        s.user.role.clear();
    } else {
        s.user.role = row["role"].as<std::string>();
    }
    if (row["created_at"].isNull()) {
        s.user.created_at.clear();
    } else {
        s.user.created_at = row["created_at"].as<std::string>();
    }
    if (row["access_token"].isNull()) {
        s.access_token.clear();
    } else {
        s.access_token = row["access_token"].as<std::string>();
    }
    if (row["access_expires_at"].isNull()) {
        s.access_expires_at.clear();
    } else {
        s.access_expires_at = row["access_expires_at"].as<std::string>();
    }
    if (row["refresh_token"].isNull()) {
        s.refresh_token.clear();
    } else {
        s.refresh_token = row["refresh_token"].as<std::string>();
    }
    if (row["refresh_expires_at"].isNull()) {
        s.refresh_expires_at.clear();
    } else {
        s.refresh_expires_at = row["refresh_expires_at"].as<std::string>();
    }
    return s;
}

const std::string &createSessionSql() {
    static const std::string sql =
        "WITH tok AS ("
        "  SELECT encode(gen_random_bytes(32), 'hex') AS raw_refresh_token, "
        "         encode(gen_random_bytes(32), 'hex') AS raw_access_token"
        "), ins AS ("
        "  INSERT INTO sessions(user_id, refresh_token_hash, expires_at, access_token_hash, access_expires_at) "
        "  SELECT $1::uuid, "
        "         encode(digest(tok.raw_refresh_token, 'sha256'), 'hex'), "
        "         now() + ($2::int * interval '1 day'), "
        "         encode(digest(tok.raw_access_token, 'sha256'), 'hex'), "
        "         now() + ($3::int * interval '1 minute') "
        "  FROM tok "
        "  RETURNING id::text AS session_id, "
        "            user_id::text AS user_id, "
        "            expires_at::text AS refresh_expires_at, "
        "            access_expires_at::text AS access_expires_at"
        ") "
        "SELECT ins.session_id, "
        "       ins.user_id, "
        "       NULL::text AS email, "
        "       NULL::text AS username, "
        "       NULL::text AS role, "
        "       NULL::text AS created_at, "
        "       tok.raw_access_token AS access_token, "
        "       ins.access_expires_at, "
        "       tok.raw_refresh_token AS refresh_token, "
        "       ins.refresh_expires_at "
        "FROM ins CROSS JOIN tok;";
    return sql;
}
}  // namespace

AuthRepository::AuthRepository(DbClientPtr db) : db_(std::move(db)) {
}

void AuthRepository::registerUser(const std::string &email,
                                  const std::string &username,
                                  const std::string &password,
                                  int accessTtlMinutes,
                                  int refreshTtlDays,
                                  std::function<void(AuthSessionRow)> onOk,
                                  std::function<void(const pm::ApiError &)> onBizErr,
                                  std::function<void(const DrogonDbException &)> onErr) const {
    if (refreshTtlDays <= 0 || accessTtlMinutes <= 0) {
        return onBizErr({drogon::k500InternalServerError, "invalid token ttl"});
    }

    struct Ctx {
        TransactionPtr tx;
        std::function<void(AuthSessionRow)> onOk;
        std::function<void(const pm::ApiError &)> onBizErr;
        std::function<void(const DrogonDbException &)> onErr;
        int refreshTtlDays{};
        int accessTtlMinutes{};
        AuthUserRow user;
    };

    auto ctx = std::make_shared<Ctx>();
    ctx->onOk = std::move(onOk);
    ctx->onBizErr = std::move(onBizErr);
    ctx->onErr = std::move(onErr);
    ctx->refreshTtlDays = refreshTtlDays;
    ctx->accessTtlMinutes = accessTtlMinutes;

    db_->newTransactionAsync([ctx, email, username, password](const TransactionPtr &tx) {
        ctx->tx = tx;

        static const std::string insertUserSql =
            "INSERT INTO users(email, username, password_hash, role) "
            "VALUES(lower($1::text), $2::text, crypt($3::text, gen_salt('bf')), 'user') "
            "ON CONFLICT DO NOTHING "
            "RETURNING id::text AS id, email, username, role, created_at::text AS created_at;";

        tx->execSqlAsync(
            insertUserSql,
            [ctx](const Result &r) mutable {
                if (r.empty()) {
                    return ctx->onBizErr({drogon::k409Conflict, "email or username already exists"});
                }

                ctx->user = rowToUser(r, 0);

                static const std::string insertWalletSql =
                    "INSERT INTO wallets(user_id) VALUES($1::uuid) ON CONFLICT DO NOTHING;";

                ctx->tx->execSqlAsync(
                    insertWalletSql,
                    [ctx](const Result &) mutable {
                        ctx->tx->execSqlAsync(
                            createSessionSql(),
                            [ctx](const Result &r2) mutable {
                                if (r2.empty()) {
                                    return ctx->onBizErr({drogon::k500InternalServerError, "failed to create session"});
                                }
                                AuthSessionRow out = rowToSession(r2, 0);
                                out.user = ctx->user;
                                ctx->onOk(std::move(out));
                            },
                            [ctx](const DrogonDbException &e) mutable {
                                ctx->onErr(e);
                            },
                            ctx->user.id,
                            ctx->refreshTtlDays,
                            ctx->accessTtlMinutes);
                    },
                    [ctx](const DrogonDbException &e) mutable {
                        ctx->onErr(e);
                    },
                    ctx->user.id);
            },
            [ctx](const DrogonDbException &e) mutable {
                ctx->onErr(e);
            },
            email,
            username,
            password);
    });
}

void AuthRepository::login(const std::string &login,
                           const std::string &password,
                           int accessTtlMinutes,
                           int refreshTtlDays,
                           std::function<void(AuthSessionRow)> onOk,
                           std::function<void(const pm::ApiError &)> onBizErr,
                           std::function<void(const DrogonDbException &)> onErr) const {
    if (refreshTtlDays <= 0 || accessTtlMinutes <= 0) {
        return onBizErr({drogon::k500InternalServerError, "invalid token ttl"});
    }

    struct Ctx {
        std::function<void(AuthSessionRow)> onOk;
        std::function<void(const pm::ApiError &)> onBizErr;
        std::function<void(const DrogonDbException &)> onErr;
        int refreshTtlDays{};
        int accessTtlMinutes{};
        AuthUserRow user;
    };

    auto ctx = std::make_shared<Ctx>();
    ctx->onOk = std::move(onOk);
    ctx->onBizErr = std::move(onBizErr);
    ctx->onErr = std::move(onErr);
    ctx->refreshTtlDays = refreshTtlDays;
    ctx->accessTtlMinutes = accessTtlMinutes;

    static const std::string findUserSql =
        "SELECT id::text AS id, email, username, role, created_at::text AS created_at "
        "FROM users "
        "WHERE (email = lower($1::text) OR username = $1::text) "
        "  AND password_hash = crypt($2::text, password_hash) "
        "LIMIT 1;";

    db_->execSqlAsync(
        findUserSql,
        [this, ctx](const Result &r) mutable {
            if (r.empty()) {
                return ctx->onBizErr({drogon::k401Unauthorized, "invalid credentials"});
            }

            ctx->user = rowToUser(r, 0);

            db_->execSqlAsync(
                createSessionSql(),
                [ctx](const Result &r2) mutable {
                    if (r2.empty()) {
                        return ctx->onBizErr({drogon::k500InternalServerError, "failed to create session"});
                    }
                    AuthSessionRow out = rowToSession(r2, 0);
                    out.user = ctx->user;
                    ctx->onOk(std::move(out));
                },
                [ctx](const DrogonDbException &e) mutable {
                    ctx->onErr(e);
                },
                ctx->user.id,
                ctx->refreshTtlDays,
                ctx->accessTtlMinutes);
        },
        [ctx](const DrogonDbException &e) mutable {
            ctx->onErr(e);
        },
        login,
        password);
}

void AuthRepository::refresh(const std::string &refreshToken,
                             int accessTtlMinutes,
                             int refreshTtlDays,
                             std::function<void(AuthSessionRow)> onOk,
                             std::function<void(const pm::ApiError &)> onBizErr,
                             std::function<void(const DrogonDbException &)> onErr) const {
    if (refreshTtlDays <= 0 || accessTtlMinutes <= 0) {
        return onBizErr({drogon::k500InternalServerError, "invalid token ttl"});
    }

    static const std::string refreshSql =
        "WITH tok AS ("
        "  SELECT encode(gen_random_bytes(32), 'hex') AS raw_refresh_token, "
        "         encode(gen_random_bytes(32), 'hex') AS raw_access_token"
        "), old_session AS ("
        "  SELECT s.id, s.user_id, u.email, u.username, u.role, u.created_at::text AS created_at "
        "  FROM sessions s "
        "  JOIN users u ON u.id = s.user_id "
        "  WHERE s.refresh_token_hash = encode(digest($1::text, 'sha256'), 'hex') "
        "    AND s.expires_at > now() "
        "  LIMIT 1"
        "), upd AS ("
        "  UPDATE sessions s "
        "  SET refresh_token_hash = encode(digest((SELECT raw_refresh_token FROM tok), 'sha256'), 'hex'), "
        "      expires_at = now() + ($2::int * interval '1 day'), "
        "      access_token_hash = encode(digest((SELECT raw_access_token FROM tok), 'sha256'), 'hex'), "
        "      access_expires_at = now() + ($3::int * interval '1 minute') "
        "  FROM old_session os "
        "  WHERE s.id = os.id "
        "  RETURNING s.id::text AS session_id, s.user_id::text AS user_id, "
        "            s.expires_at::text AS refresh_expires_at, "
        "            s.access_expires_at::text AS access_expires_at"
        ") "
        "SELECT upd.session_id, upd.user_id, os.email, os.username, os.role, os.created_at, "
        "       tok.raw_access_token AS access_token, upd.access_expires_at, "
        "       tok.raw_refresh_token AS refresh_token, upd.refresh_expires_at "
        "FROM upd "
        "JOIN old_session os ON os.user_id = upd.user_id::uuid "
        "CROSS JOIN tok;";

    db_->execSqlAsync(
        refreshSql,
        [onOk = std::move(onOk), onBizErr = std::move(onBizErr)](const Result &r) mutable {
            if (r.empty()) {
                return onBizErr({drogon::k401Unauthorized, "invalid or expired refresh token"});
            }
            onOk(rowToSession(r, 0));
        },
        std::move(onErr),
        refreshToken,
        refreshTtlDays,
        accessTtlMinutes);
}

void AuthRepository::authenticateAccessToken(const std::string &accessToken,
                                             std::function<void(AuthSessionRow)> onOk,
                                             std::function<void(const pm::ApiError &)> onBizErr,
                                             std::function<void(const DrogonDbException &)> onErr) const {
    static const std::string authSql =
        "SELECT s.id::text AS session_id, "
        "       u.id::text AS user_id, "
        "       u.email, u.username, u.role, u.created_at::text AS created_at, "
        "       NULL::text AS access_token, "
        "       s.access_expires_at::text AS access_expires_at, "
        "       NULL::text AS refresh_token, "
        "       s.expires_at::text AS refresh_expires_at "
        "FROM sessions s "
        "JOIN users u ON u.id = s.user_id "
        "WHERE s.access_token_hash = encode(digest($1::text, 'sha256'), 'hex') "
        "  AND s.access_expires_at > now() "
        "LIMIT 1;";

    db_->execSqlAsync(
        authSql,
        [onOk = std::move(onOk), onBizErr = std::move(onBizErr)](const Result &r) mutable {
            if (r.empty()) {
                return onBizErr({drogon::k401Unauthorized, "invalid or expired access token"});
            }
            onOk(rowToSession(r, 0));
        },
        std::move(onErr),
        accessToken);
}

void AuthRepository::logout(const std::string &refreshToken,
                            std::function<void()> onOk,
                            std::function<void(const DrogonDbException &)> onErr) const {
    static const std::string logoutSql =
        "DELETE FROM sessions "
        "WHERE refresh_token_hash = encode(digest($1::text, 'sha256'), 'hex');";

    db_->execSqlAsync(
        logoutSql,
        [onOk = std::move(onOk)](const Result &) mutable {
            onOk();
        },
        std::move(onErr),
        refreshToken);
}
