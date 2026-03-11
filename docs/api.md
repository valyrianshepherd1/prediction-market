# Backend REST API (v0) — Prediction Market

---

## Base URL

**Dev default:**

- `https://localhost:8443`

**Health check:**

- `GET https://localhost:8443/healthz`

> HTTPS is enabled with a **self‑signed** certificate in dev. See “TLS / self‑signed cert” below.

---

## Authentication (temporary)

### User-scoped requests

For user-specific endpoints and order actions you must send:

- Header: `X-User-Id: <string>`

Example:
```
X-User-Id: user_1
```

### Admin requests

Admin endpoints require:

- Backend environment variable: `PM_ADMIN_TOKEN=<secret>`
- Request header: `X-Admin-Token: <secret>`

Example:
```
X-Admin-Token: dev_admin_token_123
```

---

## Data conventions

### IDs
All IDs are strings (UUIDs in the database).

### Money / quantity / price

- `amount` (wallet deposit): **int64**
- `qty_micros` (order quantity): **int64**
- `price_bp` (order price): **int** in **basis points**
  - `100`  = 1.00%
  - `5000` = 50.00%
  - `10000` = 100.00%

### Error response shape

For errors, the server returns JSON:
```json
{ "error": "message" }
```

---

## Endpoint list (index)

### Health
- `GET /healthz`
- `GET /healthz/db`

### Markets
- `GET /markets`
- `GET /markets/{marketId}`
- `GET /markets/{marketId}/outcomes`
- `POST /admin/markets`

### Wallets
- `GET /wallets/{userId}`
- `POST /admin/deposit`

### Orders
- `POST /orders`
- `GET /orders/{orderId}`
- `DELETE /orders/{orderId}`
- `GET /me/orders`
- `GET /outcomes/{outcomeId}/orderbook`

### Trades
- `GET /outcomes/{outcomeId}/trades`
- `GET /me/trades`

---

# Detailed endpoint reference

## Health

### GET `/healthz`
Checks that the backend process is alive.

**Headers:** none

**Response 200**
```json
{ "status": "ok" }
```

---

### GET `/healthz/db`
Checks DB connectivity (runs a simple `SELECT 1`).

**Headers:** none

**Response 200**
```json
{ "status": "ok" }
```

**Response 503** (DB query failed)
```json
{ "status": "fail", "error": "…" }
```

**Response 500** (DB client not configured/available)
```json
{ "status": "fail", "error": "db client not available: …" }
```

---

## Markets

### GET `/markets`
Lists markets (paged), optionally filtered by status.

**Headers:** none

**Query params:**
- `limit` (int, default `50`, min `1`, max `200`)
- `offset` (int, default `0`)
- `status` (optional): `OPEN | CLOSED | RESOLVED`

**Response 200**
```json
[
  {
    "id": "…",
    "question": "Will X happen?",
    "status": "OPEN",
    "resolved_outcome_id": null,
    "created_at": "…"
  }
]
```

**Response 400** (bad paging/status)
```json
{ "error": "…" }
```

Example:
```
GET /markets?status=OPEN&limit=50&offset=0
```

---

### GET `/markets/{marketId}`
Fetch a single market by ID.

**Headers:** none

**Path params:**
- `marketId` (string)

**Response 200**
```json
{
  "id": "…",
  "question": "Will X happen?",
  "status": "OPEN",
  "resolved_outcome_id": null,
  "created_at": "…"
}
```

**Response 404**
```json
{ "error": "market not found" }
```

---

### GET `/markets/{marketId}/outcomes`
Lists outcomes for a market.

**Headers:** none

**Path params:**
- `marketId` (string)

**Response 200**
```json
[
  {
    "id": "…",
    "market_id": "…",
    "title": "YES",
    "outcome_index": 0
  },
  {
    "id": "…",
    "market_id": "…",
    "title": "NO",
    "outcome_index": 1
  }
]
```

---

### POST `/admin/markets`
Creates a new market (admin-only).

**Headers:**
- `X-Admin-Token: …`

**Body (JSON):**
- `question` (string, 3..300 chars) — required  
- `title` is accepted as an alias for `question`

Example body:
```json
{ "question": "Will X happen?" }
```

**Response 201**
```json
{
  "id": "…",
  "question": "Will X happen?",
  "status": "OPEN",
  "resolved_outcome_id": null,
  "created_at": "…"
}
```

**Response 401**
```json
{ "error": "admin token missing or invalid" }
```

**Response 400**
```json
{ "error": "expected JSON body" }
```

> Current limitation (MVP): this creates the **market row**, but outcomes may be created separately (via seeds/migrations/manual DB).  
> If you need “create market with outcomes” in one request — that’s a good next backend task.

---

## Wallets

### GET `/wallets/{userId}`
Returns wallet info for a user.

**Headers:** none (MVP)

**Path params:**
- `userId` (string)

**Response 200**
```json
{
  "user_id": "user_1",
  "available": 1000000,
  "reserved": 0,
  "updated_at": "…"
}
```

**Response 404**
```json
{ "error": "wallet not found" }
```

---

### POST `/admin/deposit`
Admin-only deposit (top-up) for a user wallet.

**Headers:**
- `X-Admin-Token: …`

**Body (JSON):**
- `user_id` (string) — required
- `amount` (int64) — can be positive (deposit). If negative is allowed depends on service validation.

Example:
```json
{ "user_id": "user_1", "amount": 1000000 }
```

**Response 200**
```json
{
  "user_id": "user_1",
  "available": 2000000,
  "reserved": 0,
  "updated_at": "…"
}
```

**Response 401**
```json
{ "error": "admin token missing or invalid" }
```

**Response 400**
```json
{ "error": "user_id is required" }
```

---

## Orders

### POST `/orders`
Creates an order and immediately attempts matching.

**Headers:**
- `X-User-Id: …` (required)

**Body (JSON):**
- `outcome_id` (string) — required
- `side` (string) — required: `"BUY"` or `"SELL"`
- `qty_micros` (int64) — required, must be > 0
- `price_bp` (int, optional, default 0)

Example:
```json
{
  "outcome_id": "…",
  "side": "BUY",
  "price_bp": 5200,
  "qty_micros": 250000
}
```

**Response 201**
```json
{
  "id": "…",
  "user_id": "user_1",
  "outcome_id": "…",
  "side": "BUY",
  "price_bp": 5200,
  "qty_total_micros": 250000,
  "qty_remaining_micros": 250000,
  "status": "OPEN",
  "created_at": "…",
  "updated_at": "…"
}
```

**Response 401**
```json
{ "error": "X-User-Id header is required (temporary auth)" }
```

**Response 400**
```json
{ "error": "outcome_id, side, qty_micros are required" }
```

---

### GET `/outcomes/{outcomeId}/orderbook`
Returns order book for the given outcome.

**Headers:** none

**Path params:**
- `outcomeId` (string)

**Query params:**
- `depth` (int, default 50)

**Response 200**
```json
{
  "buy": [ /* array of orders */ ],
  "sell": [ /* array of orders */ ]
}
```

Each order object has the same shape as in `POST /orders` response.

---

### GET `/me/orders`
Lists current user orders (paged).

**Headers:**
- `X-User-Id: …` (required)

**Query params:**
- `limit` (int, default 50)
- `offset` (int, default 0)
- `status` (optional string) — example: `OPEN`, `CANCELED`, etc.

**Response 200**
```json
[
  { "id": "…", "user_id": "…", "outcome_id": "…", "side": "BUY", "price_bp": 5200, "qty_total_micros": 250000,
    "qty_remaining_micros": 250000, "status": "OPEN", "created_at": "…", "updated_at": "…" }
]
```

---

### GET `/orders/{orderId}`
Returns order by ID, only if owned by current user.

**Headers:**
- `X-User-Id: …` (required)

**Path params:**
- `orderId` (string)

**Response 200**: order JSON

**Response 404**
```json
{ "error": "order not found" }
```

---

### DELETE `/orders/{orderId}`
Cancels an order, only if owned by current user.

**Headers:**
- `X-User-Id: …` (required)

**Path params:**
- `orderId` (string)

**Response 200**: order JSON (status changed to canceled / released in DB)

**Response 404**
```json
{ "error": "order not found" }
```

---

## Trades

### GET `/outcomes/{outcomeId}/trades`
Lists trades for an outcome (paged).

**Headers:** none

**Path params:**
- `outcomeId` (string)

**Query params:**
- `limit` (int, default 50)
- `offset` (int, default 0)

**Response 200**
```json
[
  {
    "id": "…",
    "outcome_id": "…",
    "maker_user_id": "…",
    "taker_user_id": "…",
    "maker_order_id": "…",
    "taker_order_id": "…",
    "price_bp": 5200,
    "qty_micros": 250000,
    "created_at": "…"
  }
]
```

---

### GET `/me/trades`
Lists trades for current user (paged).

**Headers:**
- `X-User-Id: …` (required)

**Query params:**
- `limit` (int, default 50)
- `offset` (int, default 0)

**Response 200**: array of trade JSON objects (same shape as above)

---

## TLS / self-signed cert (dev)

### curl (dev)
Skip verification:
```bash
curl -k https://localhost:8443/healthz
```

### Qt (dev)
Two typical approaches:

1) **Trust the certificate** (preferred dev approach):
- ship `server.crt` with the app and add it to Qt’s CA list.

2) **Disable verification** (fast, dev-only):
- connect `QNetworkReply::sslErrors` and call `reply->ignoreSslErrors()`.

> Do NOT ignore SSL errors in production.
