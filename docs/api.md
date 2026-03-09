# Backend REST API (current)

## Base URL

Dev default:

- `https://localhost:8443`

Health checks:

- `GET /healthz`
- `GET /healthz/db`

Dev uses HTTPS with a self-signed certificate, so most curl commands need `-k`.

---

## Authentication

### Bearer access token

User-scoped and admin-scoped endpoints require:

```http
Authorization: Bearer <access_token>
```

Access tokens are returned by:

- `POST /auth/register`
- `POST /auth/login`
- `POST /auth/refresh`

### Refresh token

Refresh and logout use a JSON body with `refresh_token`.

### Admin authorization

Admin endpoints require:

- a valid Bearer access token
- authenticated user role = `admin`

If a valid token is present but the role is not admin, the API returns `403`.

### Error shape

Errors use:

```json
{ "error": "message" }
```

---

## Data conventions

### IDs

All IDs are UUID strings.

### Numeric fields

- `amount`: signed int64
- `qty_micros`: int64
- `price_bp`: int (basis points)
- `delta_available`: int64
- `delta_reserved`: int64

### Market statuses

Current market statuses:

- `OPEN`
- `CLOSED`
- `RESOLVED`
- `ARCHIVED`

`GET /markets` hides archived markets by default. Use `?status=ARCHIVED` to list them explicitly.

---

## Endpoint index

### Health

- `GET /healthz`
- `GET /healthz/db`

### Auth

- `POST /auth/register`
- `POST /auth/login`
- `POST /auth/refresh`
- `POST /auth/logout`
- `GET /me`

### Markets

- `GET /markets`
- `GET /markets/{marketId}`
- `GET /markets/{marketId}/outcomes`
- `POST /admin/markets`
- `PATCH /admin/markets/{marketId}`
- `POST /admin/markets/{marketId}/close`
- `POST /admin/markets/{marketId}/resolve`
- `POST /admin/markets/{marketId}/archive`
- `DELETE /admin/markets/{marketId}/delete`

### Wallets / Portfolio

- `GET /wallet`
- `GET /wallets/{userId}`
- `POST /admin/deposit`
- `GET /portfolio`
- `GET /ledger`

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

# Detailed reference

## Health

### GET `/healthz`

Backend liveness probe.

**Auth:** none

**200**
```json
{ "status": "ok" }
```

### GET `/healthz/db`

Database connectivity probe.

**Auth:** none

**200**
```json
{ "status": "ok" }
```

**500 / 503**
```json
{ "status": "fail", "error": "..." }
```

---

## Auth

### POST `/auth/register`

Create a user and immediately open a session.

**Auth:** none

**Body**
```json
{
  "email": "user@example.com",
  "username": "user1",
  "password": "password123"
}
```

**Rules**
- `email`: email-like string
- `username`: 3..50 chars
- `password`: 8..200 chars

**201**
```json
{
  "session_id": "uuid",
  "user": {
    "id": "uuid",
    "email": "user@example.com",
    "username": "user1",
    "role": "user",
    "created_at": "..."
  },
  "token_type": "Bearer",
  "access_token": "token",
  "access_expires_at": "...",
  "refresh_token": "token",
  "refresh_expires_at": "..."
}
```

**409**
```json
{ "error": "email or username already exists" }
```

### POST `/auth/login`

Log in by `login`, `email`, or `username`.

**Auth:** none

**Body**
```json
{
  "login": "user@example.com",
  "password": "password123"
}
```

Aliases:
- `email`
- `username`

**200**
Same response shape as register.

**401**
```json
{ "error": "invalid credentials" }
```

### POST `/auth/refresh`

Refresh access + refresh tokens from a valid refresh token.

**Auth:** none

**Body**
```json
{
  "refresh_token": "token"
}
```

**200**
Same response shape as login.

**401**
```json
{ "error": "invalid or expired refresh token" }
```

### POST `/auth/logout`

Invalidate a refresh token / session.

**Auth:** none

**Body**
```json
{
  "refresh_token": "token"
}
```

**200**
```json
{ "ok": true }
```

### GET `/me`

Return the currently authenticated principal.

**Auth:** Bearer

**200**
```json
{
  "id": "uuid",
  "email": "user@example.com",
  "username": "user1",
  "role": "user",
  "created_at": "..."
}
```

**401**
```json
{ "error": "Authorization: Bearer <token> is required" }
```

---

## Markets

### GET `/markets`

List markets.

**Auth:** none

**Query params**
- `limit` (default `50`, min `1`, max `200`)
- `offset` (default `0`)
- `status` (optional): `OPEN | CLOSED | RESOLVED | ARCHIVED`

**Notes**
- If `status` is omitted, archived markets are excluded.
- If `status=ARCHIVED`, archived markets are returned explicitly.

**200**
```json
[
  {
    "id": "uuid",
    "question": "Will X happen?",
    "status": "OPEN",
    "resolved_outcome_id": null,
    "created_at": "..."
  }
]
```

### GET `/markets/{marketId}`

Get one market.

**Auth:** none

**200**
```json
{
  "id": "uuid",
  "question": "Will X happen?",
  "status": "OPEN",
  "resolved_outcome_id": null,
  "created_at": "..."
}
```

**404**
```json
{ "error": "market not found" }
```

### GET `/markets/{marketId}/outcomes`

List outcomes for a market.

**Auth:** none

**200**
```json
[
  {
    "id": "uuid",
    "market_id": "uuid",
    "title": "YES",
    "outcome_index": 0
  },
  {
    "id": "uuid",
    "market_id": "uuid",
    "title": "NO",
    "outcome_index": 1
  }
]
```

### POST `/admin/markets`

Create a market with outcomes.

**Auth:** Bearer admin

**Body**
```json
{
  "question": "Will BTC be above 100k by Dec 31?",
  "outcomes": ["YES", "NO"]
}
```

Accepted aliases:
- `title` instead of `question`
- `outcome_titles` instead of `outcomes`

If outcomes are omitted, defaults are:

```json
["YES", "NO"]
```

**Validation**
- question: 3..300 chars
- outcomes count: 2..10
- each title: 1..80 chars
- outcome titles must be unique

**201**
```json
{
  "id": "uuid",
  "question": "Will BTC be above 100k by Dec 31?",
  "status": "OPEN",
  "resolved_outcome_id": null,
  "created_at": "...",
  "outcomes": [
    {
      "id": "uuid",
      "market_id": "uuid",
      "title": "YES",
      "outcome_index": 0
    },
    {
      "id": "uuid",
      "market_id": "uuid",
      "title": "NO",
      "outcome_index": 1
    }
  ]
}
```

### PATCH `/admin/markets/{marketId}`

Update question and/or status.

**Auth:** Bearer admin

**Body**
```json
{
  "question": "New question",
  "status": "OPEN"
}
```

Allowed status values here:
- `OPEN`
- `CLOSED`

Use:
- `/resolve` for `RESOLVED`
- `/archive` for `ARCHIVED`

**200**
Returns the updated market object.

**409**
Possible examples:
```json
{ "error": "resolved market cannot be updated" }
```

### POST `/admin/markets/{marketId}/close`

Close a market.

**Auth:** Bearer admin

**200**
Returns the closed market object.

**409**
Possible examples:
```json
{ "error": "resolved market cannot be closed" }
```

### POST `/admin/markets/{marketId}/resolve`

Resolve a market and trigger settlement.

**Auth:** Bearer admin

**Body**
```json
{
  "winning_outcome_id": "uuid"
}
```

Alias accepted:
- `outcome_id`

**200**
Returns the resolved market:
```json
{
  "id": "uuid",
  "question": "...",
  "status": "RESOLVED",
  "resolved_outcome_id": "uuid",
  "created_at": "..."
}
```

**400**
```json
{ "error": "winning_outcome_id does not belong to market" }
```

### POST `/admin/markets/{marketId}/archive`

Archive a market.

**Auth:** Bearer admin

**200**
Returns the archived market object.

**409**
Possible example:
```json
{ "error": "resolved market cannot be archived" }
```

### DELETE `/admin/markets/{marketId}/delete`

Delete alias for archive.

**Auth:** Bearer admin

**Behavior**
This is a soft-delete alias: it archives the market.

**200**
Returns the archived market object.

---

## Wallets / Portfolio

### GET `/wallet`

Get the authenticated user's wallet.

**Auth:** Bearer

**200**
```json
{
  "user_id": "uuid",
  "available": 1000000,
  "reserved": 0,
  "updated_at": "..."
}
```

### GET `/wallets/{userId}`

Get any user's wallet.

**Auth:** Bearer admin

**200**
Same shape as `/wallet`.

### POST `/admin/deposit`

Admin deposit / top-up.

**Auth:** Bearer admin

**Body**
```json
{
  "user_id": "uuid",
  "amount": 1000000
}
```

**200**
Returns the updated wallet.

### GET `/portfolio`

List the authenticated user's positions.

**Auth:** Bearer

**Query params**
- `limit` (default `50`)
- `offset` (default `0`)

**200**
```json
[
  {
    "user_id": "uuid",
    "outcome_id": "uuid",
    "market_id": "uuid",
    "market_question": "Will X happen?",
    "outcome_title": "YES",
    "outcome_index": 0,
    "shares_available": 1000000,
    "shares_reserved": 0,
    "shares_total": 1000000,
    "updated_at": "..."
  }
]
```

### GET `/ledger`

Unified ledger feed for the authenticated user.

**Auth:** Bearer

**Query params**
- `limit` (default `50`)
- `offset` (default `0`)

**200**
```json
[
  {
    "id": "uuid",
    "ledger_type": "CASH",
    "kind": "SETTLEMENT",
    "outcome_id": null,
    "delta_available": 1000000,
    "delta_reserved": 0,
    "ref_type": "MARKET",
    "ref_id": "uuid",
    "created_at": "..."
  }
]
```

---

## Orders

### POST `/orders`

Create an order and immediately attempt matching.

**Auth:** Bearer

**Body**
```json
{
  "outcome_id": "uuid",
  "side": "BUY",
  "price_bp": 5200,
  "qty_micros": 250000
}
```

**Required**
- `outcome_id`
- `side`
- `qty_micros > 0`

**201**
```json
{
  "id": "uuid",
  "user_id": "uuid",
  "outcome_id": "uuid",
  "side": "BUY",
  "price_bp": 5200,
  "qty_total_micros": 250000,
  "qty_remaining_micros": 250000,
  "status": "OPEN",
  "created_at": "...",
  "updated_at": "..."
}
```

### GET `/orders/{orderId}`

Get one of the authenticated user's orders.

**Auth:** Bearer

**200**
Same shape as order creation response.

### DELETE `/orders/{orderId}`

Cancel one of the authenticated user's orders.

**Auth:** Bearer

**200**
Returns the updated order object.

### GET `/me/orders`

List the authenticated user's orders.

**Auth:** Bearer

**Query params**
- `limit` (default `50`)
- `offset` (default `0`)
- `status` (optional)

**200**
Array of order objects.

### GET `/outcomes/{outcomeId}/orderbook`

Public order book for an outcome.

**Auth:** none

**Query params**
- `depth` (default `50`)

**200**
```json
{
  "buy": [ /* order objects */ ],
  "sell": [ /* order objects */ ]
}
```

---

## Trades

### GET `/outcomes/{outcomeId}/trades`

Public trades for an outcome.

**Auth:** none

**Query params**
- `limit` (default `50`)
- `offset` (default `0`)

**200**
```json
[
  {
    "id": "uuid",
    "outcome_id": "uuid",
    "maker_user_id": "uuid",
    "taker_user_id": "uuid",
    "maker_order_id": "uuid",
    "taker_order_id": "uuid",
    "price_bp": 6000,
    "qty_micros": 1000000,
    "created_at": "..."
  }
]
```

### GET `/me/trades`

List the authenticated user's trades.

**Auth:** Bearer

**Query params**
- `limit` (default `50`)
- `offset` (default `0`)

**200**
Array of trade objects.

---

## Common auth failures

### Missing or malformed bearer token

```json
{ "error": "Authorization: Bearer <token> is required" }
```

### Valid token, but not admin

```json
{ "error": "admin role is required" }
```
