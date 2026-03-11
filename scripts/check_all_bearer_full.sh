#!/usr/bin/env bash
set -euo pipefail

# -----------------------------
# config
# -----------------------------
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BASE_URL="${PM_BASE_URL:-https://127.0.0.1:8443}"
CONFIG_PATH_DEFAULT="${ROOT_DIR}/backend/config/config.json"
CONFIG_PATH="${PM_CONFIG_PATH:-$CONFIG_PATH_DEFAULT}"

POSTGRES_CONTAINER="${PM_PG_CONTAINER:-pm_postgres}"
DB_USER_DEFAULT="pm"
DB_NAME_DEFAULT="pmdb"

DB_USER="${POSTGRES_USER:-$DB_USER_DEFAULT}"
DB_NAME="${POSTGRES_DB:-$DB_NAME_DEFAULT}"

STARTED_BACKEND=0
BACKEND_PID=""

# seeded ids / auth state for cleanup and tests
SEED_ADMIN_ID=""
SEED_BUYER_ID=""
SEED_SELLER_ID=""
SEED_LIFECYCLE_MARKET_ID=""
SEED_DELETE_MARKET_ID=""
SEED_TRADE_MARKET_ID=""
SEED_TRADE_OUTCOME_ID=""
SEED_BUY_ORDER_ID=""
SEED_SELL_ORDER_ID=""

ADMIN_EMAIL=""
ADMIN_USERNAME=""
ADMIN_PASSWORD=""
ADMIN_ACCESS_TOKEN=""
ADMIN_REFRESH_TOKEN=""

BUYER_EMAIL=""
BUYER_USERNAME=""
BUYER_PASSWORD=""
BUYER_ACCESS_TOKEN=""
BUYER_REFRESH_TOKEN=""
BUYER_ACCESS_TOKEN_REFRESHED=""

SELLER_EMAIL=""
SELLER_USERNAME=""
SELLER_PASSWORD=""
SELLER_ACCESS_TOKEN=""
SELLER_REFRESH_TOKEN=""

# -----------------------------
# helpers
# -----------------------------
say() { printf "%s\n" "$*"; }
ok() { printf "✅ %s\n" "$*"; }
warn() { printf "⚠️  %s\n" "$*"; }
err() { printf "❌ %s\n" "$*" >&2; }

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    err "Missing required command: $1"
    exit 1
  fi
}

have_cmd() { command -v "$1" >/dev/null 2>&1; }

docker_compose() {
  if have_cmd docker && docker compose version >/dev/null 2>&1; then
    docker compose "$@"
  elif have_cmd docker-compose; then
    docker-compose "$@"
  else
    err "Neither 'docker compose' nor 'docker-compose' is available."
    exit 1
  fi
}

psql_in_container() {
  local sql="$1"
  docker exec -i "${POSTGRES_CONTAINER}" \
    psql -X -q -U "${DB_USER}" -d "${DB_NAME}" -tA -c "${sql}" \
    | sed '/^[[:space:]]*$/d' \
    | head -n 1 \
    | tr -d '\r' \
    | xargs
}

new_uuid() {
  if have_cmd uuidgen; then
    uuidgen | tr '[:upper:]' '[:lower:]'
  else
    python3 - <<'PY'
import uuid
print(uuid.uuid4())
PY
  fi
}

http_call() {
  # usage: http_call METHOD URL [JSON_BODY] [HEADER1] [HEADER2] ...
  local method="$1"; shift
  local url="$1"; shift

  local tmp errf
  tmp="$(mktemp)"
  errf="$(mktemp)"

  local code="000"
  local rc=0
  local body=""

  if [[ "$method" != "GET" && $# -gt 0 && "$1" != -H && "$1" != --* ]]; then
    body="$1"
    shift
  fi

  local -a cmd=(curl -skS --max-time "${CURL_MAX_TIME:-10}" -X "$method" "$url" -o "$tmp" -w "%{http_code}")
  if [[ "$method" == "GET" ]]; then
    cmd=(curl -skS --max-time "${CURL_MAX_TIME:-10}" -o "$tmp" -w "%{http_code}" "$url")
  elif [[ -n "$body" ]]; then
    cmd+=( -H "Content-Type: application/json" -d "$body" )
  fi
  if [[ $# -gt 0 ]]; then
    cmd+=( "$@" )
  fi

  code="$("${cmd[@]}" 2>"$errf")" || rc=$?

  local resp
  resp="$(cat "$tmp")"

  if [[ $rc -ne 0 ]]; then
    echo "curl error (rc=$rc) for $method $url:" >&2
    sed 's/^/  /' "$errf" >&2
    if [[ -n "$resp" ]]; then
      echo "response body (partial/if any):" >&2
      echo "$resp" | sed 's/^/  /' >&2
    fi
  fi

  rm -f "$tmp" "$errf"
  printf "%s\n%s" "$code" "$resp"
}

json_get() {
  local json="$1"
  local key="$2"
  python3 -c 'import json,sys
j=json.loads(sys.stdin.read())
v=j.get(sys.argv[1], "")
print("" if v is None else v)
' "$key" <<< "$json"
}

json_get_json() {
  local json="$1"
  local key="$2"
  python3 -c 'import json,sys
j=json.loads(sys.stdin.read())
v=j.get(sys.argv[1], None)
print("" if v is None else json.dumps(v))
' "$key" <<< "$json"
}

json_len() {
  local json="$1"
  python3 -c 'import json,sys
v=json.loads(sys.stdin.read())
print(len(v) if isinstance(v, list) else -1)
' <<< "$json"
}

json_array_contains_field() {
  local json="$1"
  local key="$2"
  local expected="$3"
  python3 -c 'import json,sys
v=json.loads(sys.stdin.read())
k=sys.argv[1]
expected=sys.argv[2]
ok=isinstance(v, list) and any(isinstance(x, dict) and str(x.get(k, "")) == expected for x in v)
print("1" if ok else "0")
' "$key" "$expected" <<< "$json"
}

json_array_field_at() {
  local json="$1"
  local idx="$2"
  local key="$3"
  python3 -c 'import json,sys
v=json.loads(sys.stdin.read())
idx=int(sys.argv[1])
key=sys.argv[2]
if not isinstance(v, list) or idx < 0 or idx >= len(v) or not isinstance(v[idx], dict):
    print("")
else:
    val=v[idx].get(key, "")
    print("" if val is None else val)
' "$idx" "$key" <<< "$json"
}

cleanup() {
  if [[ "${STARTED_BACKEND}" == "1" && -n "${BACKEND_PID}" ]]; then
    kill "${BACKEND_PID}" >/dev/null 2>&1 || true
  fi

  if [[ -n "${SEED_LIFECYCLE_MARKET_ID}" ]]; then
    psql_in_container "DELETE FROM markets WHERE id='${SEED_LIFECYCLE_MARKET_ID}'::uuid;" >/dev/null 2>&1 || true
  fi
  if [[ -n "${SEED_DELETE_MARKET_ID}" ]]; then
    psql_in_container "DELETE FROM markets WHERE id='${SEED_DELETE_MARKET_ID}'::uuid;" >/dev/null 2>&1 || true
  fi
  if [[ -n "${SEED_TRADE_MARKET_ID}" ]]; then
    psql_in_container "DELETE FROM markets WHERE id='${SEED_TRADE_MARKET_ID}'::uuid;" >/dev/null 2>&1 || true
  fi

  for uid in "${SEED_ADMIN_ID}" "${SEED_BUYER_ID}" "${SEED_SELLER_ID}"; do
    if [[ -n "$uid" ]]; then
      psql_in_container "DELETE FROM sessions WHERE user_id='${uid}'::uuid;" >/dev/null 2>&1 || true
      psql_in_container "DELETE FROM users WHERE id='${uid}'::uuid;" >/dev/null 2>&1 || true
    fi
  done
}
trap cleanup EXIT

# -----------------------------
# args
# -----------------------------
DEEP=0
if [[ "${1:-}" == "--deep" ]]; then
  DEEP=1
fi

# -----------------------------
# preflight
# -----------------------------
need_cmd docker
need_cmd curl
need_cmd python3

ENV_FILE="${ROOT_DIR}/deploy/.env"
if [[ -f "${ENV_FILE}" ]]; then
  set -a
  # shellcheck disable=SC1090
  source "${ENV_FILE}"
  set +a
  DB_USER="${POSTGRES_USER:-$DB_USER_DEFAULT}"
  DB_NAME="${POSTGRES_DB:-$DB_NAME_DEFAULT}"
  ok "Loaded env from deploy/.env"
else
  warn "deploy/.env not found (ok). Will use defaults."
fi

# -----------------------------
# DB checks
# -----------------------------
say "== DB checks =="

if ! docker inspect "${POSTGRES_CONTAINER}" >/dev/null 2>&1; then
  warn "Container ${POSTGRES_CONTAINER} not found. Trying to start via deploy/docker-compose.yml ..."
  (cd "${ROOT_DIR}/deploy" && docker_compose up -d)
fi

status="$(docker inspect -f '{{.State.Health.Status}}' "${POSTGRES_CONTAINER}" 2>/dev/null || true)"
if [[ "${status}" != "healthy" ]]; then
  warn "Postgres health = '${status}'. Waiting up to ~60s..."
  for _ in $(seq 1 60); do
    status="$(docker inspect -f '{{.State.Health.Status}}' "${POSTGRES_CONTAINER}" 2>/dev/null || true)"
    [[ "${status}" == "healthy" ]] && break
    sleep 1
  done
fi

status="$(docker inspect -f '{{.State.Health.Status}}' "${POSTGRES_CONTAINER}" 2>/dev/null || true)"
if [[ "${status}" != "healthy" ]]; then
  err "Postgres is not healthy. docker logs ${POSTGRES_CONTAINER} may help."
  exit 1
fi
ok "Postgres container is healthy (${POSTGRES_CONTAINER})"

required_tables=(
  users sessions markets outcomes
  wallets cash_ledger
  positions shares_ledger
  orders trades
  idempotency_keys
  market_resolutions settlements outbox_events
)
missing=0
for t in "${required_tables[@]}"; do
  exists="$(psql_in_container "SELECT to_regclass('public.${t}') IS NOT NULL;")"
  if [[ "${exists}" == "t" ]]; then
    ok "table ${t}: OK"
  else
    err "table ${t}: MISSING"
    missing=1
  fi
done

if [[ "${missing}" == "1" ]]; then
  err "Some tables are missing. Apply migrations before running deep checks."
  exit 1
fi

access_cols_ok="$(psql_in_container "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema='public' AND table_name='sessions' AND column_name IN ('access_token_hash','access_expires_at');")"
if [[ "${access_cols_ok}" == "2" ]]; then
  ok "sessions access-token columns: OK"
else
  err "sessions access-token columns are missing. Did you apply bearer-auth migration?"
  exit 1
fi

# -----------------------------
# backend checks (HTTPS)
# -----------------------------
say ""
say "== Backend checks =="

if curl -sk --max-time 1 "${BASE_URL}/healthz" >/dev/null 2>&1; then
  ok "Backend already running at ${BASE_URL}"
else
  CERT="${ROOT_DIR}/backend/config/certs/server.crt"
  KEY="${ROOT_DIR}/backend/config/certs/server.key"
  mkdir -p "${ROOT_DIR}/backend/config/logs"

  if [[ ! -f "${CERT}" || ! -f "${KEY}" ]]; then
    warn "TLS certs not found. Generating dev certs..."
    "${ROOT_DIR}/scripts/gen_dev_certs.sh"
  fi

  BIN="${PM_BACKEND_BIN:-}"
  if [[ -z "${BIN}" ]]; then
    BIN="$(find "${ROOT_DIR}" \
      -path "${ROOT_DIR}/**/vcpkg_installed" -prune -o \
      -path "${ROOT_DIR}/.git" -prune -o \
      -type f -name "backend_app" -print 2>/dev/null | head -n 1 || true)"
  fi

  if [[ -z "${BIN}" || ! -f "${BIN}" ]]; then
    err "backend_app not found. Build it or set PM_BACKEND_BIN=/path/to/backend_app."
    exit 1
  fi

  ok "Using backend binary: ${BIN}"
  ok "Using config: ${CONFIG_PATH}"

  LOG_FILE="${ROOT_DIR}/.pm_check_backend.log"
  "${BIN}" --config "${CONFIG_PATH}" >"${LOG_FILE}" 2>&1 &
  BACKEND_PID="$!"
  STARTED_BACKEND=1

  for _ in $(seq 1 60); do
    if curl -sk --max-time 1 "${BASE_URL}/healthz" >/dev/null 2>&1; then
      ok "Backend started at ${BASE_URL}"
      break
    fi
    sleep 1
  done

  if ! curl -sk --max-time 1 "${BASE_URL}/healthz" >/dev/null 2>&1; then
    err "Backend did not start. See log: ${LOG_FILE}"
    tail -n 120 "${LOG_FILE}" || true
    exit 1
  fi
fi

code_and_body="$(http_call GET "${BASE_URL}/healthz")"
code="${code_and_body%%$'\n'*}"
body="${code_and_body#*$'\n'}"
[[ "${code}" == "200" ]] && ok "GET /healthz -> 200" || { err "GET /healthz -> ${code} body=${body}"; exit 1; }

code_and_body="$(http_call GET "${BASE_URL}/healthz/db")"
code="${code_and_body%%$'\n'*}"
body="${code_and_body#*$'\n'}"
[[ "${code}" == "200" ]] && ok "GET /healthz/db -> 200" || { err "GET /healthz/db -> ${code} body=${body}"; exit 1; }

code_and_body="$(http_call GET "${BASE_URL}/markets")"
code="${code_and_body%%$'\n'*}"
body="${code_and_body#*$'\n'}"
if [[ "${code}" == "200" ]]; then
  n="$(json_len "$body" || echo -1)"
  ok "GET /markets -> 200 (len=${n})"
else
  err "GET /markets -> ${code} body=${body}"
  exit 1
fi

# -----------------------------
# deep test: auth + admin + trading + lifecycle + settlement
# -----------------------------
if [[ "${DEEP}" == "1" ]]; then
  say ""
  say "== Deep checks (auth + admin + matching + settlement) =="

  # Seed admin directly in DB, then obtain bearer token via /auth/login.
  u_admin="$(new_uuid)"
  ADMIN_EMAIL="admin_${u_admin}@example.com"
  ADMIN_USERNAME="admin_${u_admin}"
  ADMIN_PASSWORD="AdminPass_123!"
  SEED_ADMIN_ID="$(psql_in_container "INSERT INTO users (email, username, password_hash, role) VALUES ('${ADMIN_EMAIL}','${ADMIN_USERNAME}', crypt('${ADMIN_PASSWORD}', gen_salt('bf')), 'admin') RETURNING id::text;")"
  ok "Seed admin user created: ${SEED_ADMIN_ID}"

  # Register buyer via API
  u_buy="$(new_uuid)"
  BUYER_EMAIL="buyer_${u_buy}@example.com"
  BUYER_USERNAME="buyer_${u_buy}"
  BUYER_PASSWORD="BuyerPass_123!"
  body="{\"email\":\"${BUYER_EMAIL}\",\"username\":\"${BUYER_USERNAME}\",\"password\":\"${BUYER_PASSWORD}\"}"
  code_and_body="$(http_call POST "${BASE_URL}/auth/register" "$body")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  if [[ "${code}" != "201" ]]; then
    err "auth/register buyer failed: ${code} ${resp}"
    exit 1
  fi
  SEED_BUYER_ID="$(json_get_json "$resp" "user" | python3 -c 'import json,sys; j=json.loads(sys.stdin.read()); print(j.get("id",""))')"
  BUYER_ACCESS_TOKEN="$(json_get "$resp" "access_token")"
  BUYER_REFRESH_TOKEN="$(json_get "$resp" "refresh_token")"
  [[ -n "${SEED_BUYER_ID}" && -n "${BUYER_ACCESS_TOKEN}" && -n "${BUYER_REFRESH_TOKEN}" ]] \
    && ok "POST /auth/register (buyer) -> 201" \
    || { err "buyer register response missing fields: ${resp}"; exit 1; }

  # Duplicate register should fail
  code_and_body="$(http_call POST "${BASE_URL}/auth/register" "$body")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "409" ]] && ok "POST /auth/register duplicate -> 409" || { err "duplicate register expected 409, got ${code} ${resp}"; exit 1; }

  # Register seller via API
  u_sell="$(new_uuid)"
  SELLER_EMAIL="seller_${u_sell}@example.com"
  SELLER_USERNAME="seller_${u_sell}"
  SELLER_PASSWORD="SellerPass_123!"
  body="{\"email\":\"${SELLER_EMAIL}\",\"username\":\"${SELLER_USERNAME}\",\"password\":\"${SELLER_PASSWORD}\"}"
  code_and_body="$(http_call POST "${BASE_URL}/auth/register" "$body")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  if [[ "${code}" != "201" ]]; then
    err "auth/register seller failed: ${code} ${resp}"
    exit 1
  fi
  SEED_SELLER_ID="$(json_get_json "$resp" "user" | python3 -c 'import json,sys; j=json.loads(sys.stdin.read()); print(j.get("id",""))')"
  SELLER_ACCESS_TOKEN="$(json_get "$resp" "access_token")"
  SELLER_REFRESH_TOKEN="$(json_get "$resp" "refresh_token")"
  [[ -n "${SEED_SELLER_ID}" && -n "${SELLER_ACCESS_TOKEN}" && -n "${SELLER_REFRESH_TOKEN}" ]] \
    && ok "POST /auth/register (seller) -> 201" \
    || { err "seller register response missing fields: ${resp}"; exit 1; }

  # Login admin/buyer/seller
  body="{\"login\":\"${ADMIN_EMAIL}\",\"password\":\"${ADMIN_PASSWORD}\"}"
  code_and_body="$(http_call POST "${BASE_URL}/auth/login" "$body")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] || { err "admin login failed: ${code} ${resp}"; exit 1; }
  ADMIN_ACCESS_TOKEN="$(json_get "$resp" "access_token")"
  ADMIN_REFRESH_TOKEN="$(json_get "$resp" "refresh_token")"
  [[ -n "${ADMIN_ACCESS_TOKEN}" ]] && ok "POST /auth/login (admin) -> 200" || { err "admin login missing access_token: ${resp}"; exit 1; }

  body="{\"login\":\"${BUYER_EMAIL}\",\"password\":\"${BUYER_PASSWORD}\"}"
  code_and_body="$(http_call POST "${BASE_URL}/auth/login" "$body")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "POST /auth/login (buyer) -> 200" || { err "buyer login failed: ${code} ${resp}"; exit 1; }

  body="{\"login\":\"${SELLER_EMAIL}\",\"password\":\"${SELLER_PASSWORD}\"}"
  code_and_body="$(http_call POST "${BASE_URL}/auth/login" "$body")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "POST /auth/login (seller) -> 200" || { err "seller login failed: ${code} ${resp}"; exit 1; }

  body="{\"login\":\"${BUYER_EMAIL}\",\"password\":\"WrongPass_123!\"}"
  code_and_body="$(http_call POST "${BASE_URL}/auth/login" "$body")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "401" ]] && ok "POST /auth/login bad password -> 401" || { err "bad login expected 401, got ${code} ${resp}"; exit 1; }

  # /me and admin role checks
  code_and_body="$(http_call GET "${BASE_URL}/me" -H "Authorization: Bearer ${BUYER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  if [[ "${code}" == "200" ]]; then
    got_id="$(json_get "$resp" "id")"
    [[ "${got_id}" == "${SEED_BUYER_ID}" ]] && ok "GET /me (buyer) -> 200" || { err "GET /me buyer unexpected body: ${resp}"; exit 1; }
  else
    err "GET /me buyer failed: ${code} ${resp}"
    exit 1
  fi

  code_and_body="$(http_call GET "${BASE_URL}/wallet" -H "Authorization: Bearer ${BUYER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /wallet (buyer) -> 200" || { err "wallet buyer failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call POST "${BASE_URL}/admin/deposit" '{"user_id":"bogus","amount":1}' -H "Authorization: Bearer ${BUYER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "403" ]] && ok "Admin endpoint with buyer Bearer token -> 403" || { err "expected 403 for buyer on admin endpoint, got ${code} ${resp}"; exit 1; }

  # Refresh buyer token
  body="{\"refresh_token\":\"${BUYER_REFRESH_TOKEN}\"}"
  code_and_body="$(http_call POST "${BASE_URL}/auth/refresh" "$body")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  if [[ "${code}" == "200" ]]; then
    BUYER_ACCESS_TOKEN_REFRESHED="$(json_get "$resp" "access_token")"
    BUYER_REFRESH_TOKEN="$(json_get "$resp" "refresh_token")"
    [[ -n "${BUYER_ACCESS_TOKEN_REFRESHED}" && -n "${BUYER_REFRESH_TOKEN}" ]] \
      && ok "POST /auth/refresh (buyer) -> 200" \
      || { err "buyer refresh response missing tokens: ${resp}"; exit 1; }
  else
    err "buyer refresh failed: ${code} ${resp}"
    exit 1
  fi
  BUYER_ACCESS_TOKEN="${BUYER_ACCESS_TOKEN_REFRESHED}"

  # Admin deposits via Bearer admin
  body="{\"user_id\":\"${SEED_BUYER_ID}\",\"amount\":1000000000}"
  code_and_body="$(http_call POST "${BASE_URL}/admin/deposit" "$body" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "POST /admin/deposit (buyer) -> 200" || { err "deposit buyer failed: ${code} ${resp}"; exit 1; }

  body="{\"user_id\":\"${SEED_SELLER_ID}\",\"amount\":1000000000}"
  code_and_body="$(http_call POST "${BASE_URL}/admin/deposit" "$body" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "POST /admin/deposit (seller) -> 200" || { err "deposit seller failed: ${code} ${resp}"; exit 1; }

  # --- lifecycle market (create/update/close/reopen/archive/filter) ---
  q_life="PM lifecycle market $(date +%s)"
  body="{\"question\":\"${q_life}\"}"
  code_and_body="$(http_call POST "${BASE_URL}/admin/markets" "$body" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "201" ]] || { err "admin create lifecycle market failed: ${code} ${resp}"; exit 1; }
  SEED_LIFECYCLE_MARKET_ID="$(json_get "$resp" "id")"
  outcomes_json="$(json_get_json "$resp" "outcomes")"
  [[ -n "${SEED_LIFECYCLE_MARKET_ID}" && "$(json_len "$outcomes_json")" -ge 2 ]] \
    && ok "POST /admin/markets (lifecycle) -> 201" \
    || { err "lifecycle market create unexpected payload: ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/markets/${SEED_LIFECYCLE_MARKET_ID}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /markets/{id} -> 200" || { err "get lifecycle market failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/markets/${SEED_LIFECYCLE_MARKET_ID}/outcomes")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  if [[ "${code}" == "200" ]]; then
    n="$(json_len "$resp" || echo -1)"
    [[ "$n" -ge 2 ]] && ok "GET /markets/{id}/outcomes -> 200" || { err "lifecycle outcomes unexpected payload: ${resp}"; exit 1; }
  else
    err "lifecycle outcomes failed: ${code} ${resp}"
    exit 1
  fi

  patched_q="${q_life} [edited]"
  body="{\"question\":\"${patched_q}\"}"
  code_and_body="$(http_call PATCH "${BASE_URL}/admin/markets/${SEED_LIFECYCLE_MARKET_ID}" "$body" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  if [[ "${code}" == "200" ]]; then
    got_q="$(json_get "$resp" "question")"
    [[ "${got_q}" == "${patched_q}" ]] && ok "PATCH /admin/markets/{id} -> 200 (question updated)" || { err "patch lifecycle question unexpected: ${resp}"; exit 1; }
  else
    err "patch lifecycle market failed: ${code} ${resp}"
    exit 1
  fi

  code_and_body="$(http_call POST "${BASE_URL}/admin/markets/${SEED_LIFECYCLE_MARKET_ID}/close" "{}" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" && "$(json_get "$resp" "status")" == "CLOSED" ]] && ok "POST /admin/markets/{id}/close -> 200" || { err "close lifecycle market failed: ${code} ${resp}"; exit 1; }

  body='{"status":"OPEN"}'
  code_and_body="$(http_call PATCH "${BASE_URL}/admin/markets/${SEED_LIFECYCLE_MARKET_ID}" "$body" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" && "$(json_get "$resp" "status")" == "OPEN" ]] && ok "PATCH /admin/markets/{id} -> 200 (reopened)" || { err "reopen lifecycle market failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call POST "${BASE_URL}/admin/markets/${SEED_LIFECYCLE_MARKET_ID}/archive" "{}" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" && "$(json_get "$resp" "status")" == "ARCHIVED" ]] && ok "POST /admin/markets/{id}/archive -> 200" || { err "archive lifecycle market failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/markets?limit=100")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  if [[ "${code}" == "200" ]]; then
    has_archived="$(json_array_contains_field "$resp" "id" "$SEED_LIFECYCLE_MARKET_ID")"
    [[ "${has_archived}" == "0" ]] && ok "GET /markets hides ARCHIVED by default" || { err "archived market visible in default list: ${resp}"; exit 1; }
  else
    err "GET /markets after archive failed: ${code} ${resp}"
    exit 1
  fi

  code_and_body="$(http_call GET "${BASE_URL}/markets?status=ARCHIVED&limit=100")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  if [[ "${code}" == "200" ]]; then
    has_archived="$(json_array_contains_field "$resp" "id" "$SEED_LIFECYCLE_MARKET_ID")"
    [[ "${has_archived}" == "1" ]] && ok "GET /markets?status=ARCHIVED -> archived market visible" || { err "archived market missing from ARCHIVED filter: ${resp}"; exit 1; }
  else
    err "GET /markets?status=ARCHIVED failed: ${code} ${resp}"
    exit 1
  fi

  # DELETE alias should also archive
  q_del="PM delete-alias market $(date +%s)"
  body="{\"question\":\"${q_del}\"}"
  code_and_body="$(http_call POST "${BASE_URL}/admin/markets" "$body" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "201" ]] || { err "admin create delete-alias market failed: ${code} ${resp}"; exit 1; }
  SEED_DELETE_MARKET_ID="$(json_get "$resp" "id")"
  [[ -n "${SEED_DELETE_MARKET_ID}" ]] && ok "Second market created for DELETE alias" || { err "delete-alias market id missing: ${resp}"; exit 1; }

  code_and_body="$(http_call DELETE "${BASE_URL}/admin/markets/${SEED_DELETE_MARKET_ID}" "{}" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" && "$(json_get "$resp" "status")" == "ARCHIVED" ]] && ok "DELETE /admin/markets/{id} -> 200 (archive alias)" || { err "delete alias failed: ${code} ${resp}"; exit 1; }

  # --- trading market + matching + portfolio + settlement ---
  q_trade="PM deep-check trade market $(date +%s)"
  body="{\"question\":\"${q_trade}\"}"
  code_and_body="$(http_call POST "${BASE_URL}/admin/markets" "$body" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "201" ]] || { err "admin create trade market failed: ${code} ${resp}"; exit 1; }
  SEED_TRADE_MARKET_ID="$(json_get "$resp" "id")"
  outcomes_json="$(json_get_json "$resp" "outcomes")"
  SEED_TRADE_OUTCOME_ID="$(json_array_field_at "$outcomes_json" 0 "id")"
  first_outcome_title="$(json_array_field_at "$outcomes_json" 0 "title")"
  [[ -n "${SEED_TRADE_MARKET_ID}" && -n "${SEED_TRADE_OUTCOME_ID}" && "$(json_len "$outcomes_json")" -ge 2 ]] \
    && ok "Trade market created with outcomes via API" \
    || { err "trade market create unexpected payload: ${resp}"; exit 1; }
  ok "Trading outcome_id=${SEED_TRADE_OUTCOME_ID}, title=${first_outcome_title}"

  # Mint seller shares directly in DB (no public mint endpoint yet)
  psql_in_container "INSERT INTO positions (user_id, outcome_id, shares_available, shares_reserved) VALUES ('${SEED_SELLER_ID}'::uuid, '${SEED_TRADE_OUTCOME_ID}'::uuid, 5000000, 0) ON CONFLICT (user_id, outcome_id) DO UPDATE SET shares_available=EXCLUDED.shares_available;" >/dev/null
  psql_in_container "INSERT INTO shares_ledger (user_id, outcome_id, kind, delta_available, delta_reserved, ref_type, ref_id) VALUES ('${SEED_SELLER_ID}'::uuid, '${SEED_TRADE_OUTCOME_ID}'::uuid, 'MINT', 5000000, 0, 'DEV_SEED', gen_random_uuid());" >/dev/null
  ok "Seller minted shares: 5_000_000"

  # wallet admin endpoint
  code_and_body="$(http_call GET "${BASE_URL}/wallets/${SEED_BUYER_ID}" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /wallets/{buyer} -> 200 (admin)" || { err "admin wallet endpoint failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/outcomes/${SEED_TRADE_OUTCOME_ID}/orderbook")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /outcomes/{id}/orderbook -> 200" || { err "orderbook failed: ${code} ${resp}"; exit 1; }

  buyer_available_before_resolve="$(psql_in_container "SELECT available FROM wallets WHERE user_id='${SEED_BUYER_ID}'::uuid;")"

  buy_body="{\"outcome_id\":\"${SEED_TRADE_OUTCOME_ID}\",\"side\":\"BUY\",\"price_bp\":6000,\"qty_micros\":1000000}"
  code_and_body="$(http_call POST "${BASE_URL}/orders" "$buy_body" -H "Authorization: Bearer ${BUYER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; buy_resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "201" ]] || { err "BUY order failed: ${code} ${buy_resp}"; exit 1; }
  SEED_BUY_ORDER_ID="$(json_get "$buy_resp" "id")"
  [[ -n "${SEED_BUY_ORDER_ID}" ]] && ok "BUY order created" || { err "BUY order id missing: ${buy_resp}"; exit 1; }

  sell_body="{\"outcome_id\":\"${SEED_TRADE_OUTCOME_ID}\",\"side\":\"SELL\",\"price_bp\":6000,\"qty_micros\":1000000}"
  code_and_body="$(http_call POST "${BASE_URL}/orders" "$sell_body" -H "Authorization: Bearer ${SELLER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; sell_resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "201" ]] || { err "SELL order failed: ${code} ${sell_resp}"; exit 1; }
  SEED_SELL_ORDER_ID="$(json_get "$sell_resp" "id")"
  [[ -n "${SEED_SELL_ORDER_ID}" ]] && ok "SELL order created" || { err "SELL order id missing: ${sell_resp}"; exit 1; }

  trades_cnt="$(psql_in_container "SELECT count(*) FROM trades WHERE outcome_id='${SEED_TRADE_OUTCOME_ID}'::uuid;")"
  [[ "${trades_cnt}" -ge 1 ]] && ok "Trades created: ${trades_cnt}" || { err "No trades found after matching."; exit 1; }

  # order/trade read endpoints
  code_and_body="$(http_call GET "${BASE_URL}/orders/${SEED_BUY_ORDER_ID}" -H "Authorization: Bearer ${BUYER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /orders/{buy_id} -> 200" || { err "get buy order failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/orders/${SEED_SELL_ORDER_ID}" -H "Authorization: Bearer ${SELLER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /orders/{sell_id} -> 200" || { err "get sell order failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/me/orders" -H "Authorization: Bearer ${BUYER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /me/orders (buyer) -> 200" || { err "me/orders buyer failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/me/orders" -H "Authorization: Bearer ${SELLER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /me/orders (seller) -> 200" || { err "me/orders seller failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/outcomes/${SEED_TRADE_OUTCOME_ID}/trades")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  if [[ "${code}" == "200" ]]; then
    n="$(json_len "$resp" || echo -1)"
    has_maker="$(json_array_contains_field "$resp" "maker_order_id" "$SEED_BUY_ORDER_ID")"
    has_taker="$(json_array_contains_field "$resp" "taker_order_id" "$SEED_SELL_ORDER_ID")"
    [[ "$n" -ge 1 && "$has_maker" == "1" && "$has_taker" == "1" ]] \
      && ok "GET /outcomes/{id}/trades -> 200" \
      || { err "outcome trades unexpected payload: ${resp}"; exit 1; }
  else
    err "outcome trades failed: ${code} ${resp}"
    exit 1
  fi

  code_and_body="$(http_call GET "${BASE_URL}/me/trades" -H "Authorization: Bearer ${BUYER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /me/trades (buyer) -> 200" || { err "me/trades buyer failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/me/trades" -H "Authorization: Bearer ${SELLER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /me/trades (seller) -> 200" || { err "me/trades seller failed: ${code} ${resp}"; exit 1; }

  # portfolio endpoints
  code_and_body="$(http_call GET "${BASE_URL}/wallet" -H "Authorization: Bearer ${BUYER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /wallet (buyer) -> 200" || { err "wallet buyer failed after trade: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/wallet" -H "Authorization: Bearer ${SELLER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /wallet (seller) -> 200" || { err "wallet seller failed after trade: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/portfolio?limit=20" -H "Authorization: Bearer ${BUYER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  if [[ "${code}" == "200" ]]; then
    has_outcome="$(json_array_contains_field "$resp" "outcome_id" "$SEED_TRADE_OUTCOME_ID")"
    [[ "$has_outcome" == "1" ]] && ok "GET /portfolio (buyer) -> 200" || { err "buyer portfolio missing trade outcome: ${resp}"; exit 1; }
  else
    err "portfolio buyer failed: ${code} ${resp}"
    exit 1
  fi

  code_and_body="$(http_call GET "${BASE_URL}/portfolio?limit=20" -H "Authorization: Bearer ${SELLER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /portfolio (seller) -> 200" || { err "portfolio seller failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/ledger?limit=20" -H "Authorization: Bearer ${BUYER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /ledger (buyer) -> 200" || { err "ledger buyer failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/ledger?limit=20" -H "Authorization: Bearer ${SELLER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "GET /ledger (seller) -> 200" || { err "ledger seller failed: ${code} ${resp}"; exit 1; }

  # resolve + settlement on winning outcome = first outcome (buyer owns traded shares)
  body="{\"winning_outcome_id\":\"${SEED_TRADE_OUTCOME_ID}\"}"
  code_and_body="$(http_call POST "${BASE_URL}/admin/markets/${SEED_TRADE_MARKET_ID}/resolve" "$body" -H "Authorization: Bearer ${ADMIN_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" && "$(json_get "$resp" "status")" == "RESOLVED" ]] && ok "POST /admin/markets/{id}/resolve -> 200" || { err "resolve trade market failed: ${code} ${resp}"; exit 1; }

  resolution_cnt="$(psql_in_container "SELECT count(*) FROM market_resolutions WHERE market_id='${SEED_TRADE_MARKET_ID}'::uuid;")"
  settlement_cnt="$(psql_in_container "SELECT count(*) FROM settlements WHERE market_id='${SEED_TRADE_MARKET_ID}'::uuid AND user_id='${SEED_BUYER_ID}'::uuid;")"
  buyer_available_after_resolve="$(psql_in_container "SELECT available FROM wallets WHERE user_id='${SEED_BUYER_ID}'::uuid;")"
  [[ "${resolution_cnt}" -ge 1 ]] && ok "market_resolutions row created" || { err "market_resolutions row missing"; exit 1; }
  [[ "${settlement_cnt}" -ge 1 ]] && ok "settlement row created for buyer" || { err "settlement row missing for buyer"; exit 1; }
  if [[ "${buyer_available_after_resolve}" -gt "${buyer_available_before_resolve}" ]]; then
    ok "Buyer wallet increased after settlement"
  else
    err "Buyer wallet did not increase after settlement (${buyer_available_before_resolve} -> ${buyer_available_after_resolve})"
    exit 1
  fi

  # logout at the end using refreshed buyer refresh token; refresh must then fail
  body="{\"refresh_token\":\"${BUYER_REFRESH_TOKEN}\"}"
  code_and_body="$(http_call POST "${BASE_URL}/auth/logout" "$body")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "200" ]] && ok "POST /auth/logout (buyer) -> 200" || { err "buyer logout failed: ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call POST "${BASE_URL}/auth/refresh" "$body")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "401" ]] && ok "POST /auth/refresh after logout -> 401" || { err "refresh after logout expected 401, got ${code} ${resp}"; exit 1; }

  code_and_body="$(http_call GET "${BASE_URL}/me" -H "Authorization: Bearer ${BUYER_ACCESS_TOKEN}")"
  code="${code_and_body%%$'\n'*}"; resp="${code_and_body#*$'\n'}"
  [[ "${code}" == "401" ]] && ok "GET /me with logged-out access token -> 401" || { err "expected 401 for old access token after logout, got ${code} ${resp}"; exit 1; }

  ok "Deep checks completed (seeded data will be cleaned up automatically)."
fi

say ""
ok "All checks passed."
say "Base URL: ${BASE_URL}"
