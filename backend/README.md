# Backend (Drogon + PostgreSQL)

This folder contains the HTTPS Drogon backend (`backend_app`) for the prediction market project.

## What is implemented

The current REST API covers:

- health checks
- authentication with Bearer access tokens + refresh tokens
- market lifecycle:
  - list / get
  - outcomes
  - create
  - update
  - close
  - resolve + settlement
  - archive
  - delete alias (`DELETE /admin/markets/{id}/delete`)
- wallets:
  - self wallet
  - admin wallet lookup
  - admin deposit
- orders:
  - create
  - get
  - cancel
  - list current user orders
  - public orderbook
- trades:
  - public trades by outcome
  - current user trades
- portfolio:
  - current positions
  - unified ledger feed

## Stack

- C++ / Drogon
- PostgreSQL
- CMake + Ninja
- vcpkg
- HTTPS enabled in dev with self-signed certificates

---

## Quick smoke check

After the backend and database are running:

```bash
curl -k https://localhost:8443/healthz
curl -k https://localhost:8443/healthz/db
curl -k https://localhost:8443/markets
```

---

## Authentication model

The API now uses **Bearer access tokens** for user-scoped and admin-scoped requests.

### User auth flow

1. `POST /auth/register` or `POST /auth/login`
2. Read `access_token` and `refresh_token` from the JSON response
3. Send:
   ```http
   Authorization: Bearer <access_token>
   ```
4. When access token expires, call `POST /auth/refresh` with the refresh token
5. To invalidate a session, call `POST /auth/logout`

### Admin auth

Admin endpoints do **not** use `X-Admin-Token` anymore.

A request is treated as admin-only when:

- it has a valid Bearer access token
- the authenticated user has `role = admin`

If you need an admin user in dev, update the user's role in PostgreSQL, for example:

```sql
UPDATE users
SET role = 'admin'
WHERE email = 'admin@example.com';
```

### Token TTL environment variables

Optional environment variables:

```bash
PM_ACCESS_TTL_MINUTES=15
PM_REFRESH_TTL_DAYS=30
```

If unset, the backend uses those defaults.

---

## macOS

### 1) Prerequisites

Install:

- Xcode / Command Line Tools
- CMake (>= 3.26)
- Ninja
- Docker Desktop
- OpenSSL
- autoconf

Homebrew example:

```bash
brew install cmake ninja autoconf openssl
```

### 2) Clone + init submodules

```bash
git clone --recurse-submodules https://github.com/valyrianshepherd1/prediction-market.git
cd prediction-market

# if needed:
git submodule update --init --recursive
```

### 3) Bootstrap vcpkg

```bash
./extern/vcpkg/bootstrap-vcpkg.sh
```

### 4) Generate dev HTTPS certs

```bash
chmod +x scripts/gen_dev_certs.sh
./scripts/gen_dev_certs.sh
```

Generated files:

- `backend/config/certs/server.crt`
- `backend/config/certs/server.key`

### 5) Start Postgres + apply migrations

```bash
chmod +x scripts/dev_db_up.sh
./scripts/dev_db_up.sh
```

Optional seed script:

```bash
chmod +x scripts/db_seed_dev.sh
./scripts/db_seed_dev.sh
```

### 6) Build backend

Recommended backend-only configure/build:

```bash
cmake -S backend -B build-backend-macos -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/extern/vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=arm64-osx

cmake --build build-backend-macos
```

### 7) Run backend

```bash
./build-backend-macos/backend_app
```

Explicit config example:

```bash
./build-backend-macos/backend_app --config "$PWD/backend/config/config.json"
```

---

## Windows

### 1) Prerequisites

Install:

- CMake (>= 3.26)
- Ninja
- Git
- Docker Desktop
- OpenSSL
- PowerShell

### 2) Clone + init submodules

```powershell
git clone --recurse-submodules https://github.com/valyrianshepherd1/prediction-market.git
cd prediction-market
git submodule update --init --recursive
```

### 3) Bootstrap vcpkg

```powershell
.\extern\vcpkg\bootstrap-vcpkg.bat
```

### 4) Generate dev HTTPS certs

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\gen_dev_certs.ps1
```

### 5) Start Postgres + apply migrations

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\dev_db_up.ps1
```

Or manually:

```powershell
cd .\deploy
docker compose up -d
cd ..
powershell -ExecutionPolicy Bypass -File .\scripts\db_migrate.ps1
```

### 6) Build backend

```powershell
cmake -S backend -B build-backend-windows -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE="$pwd\extern\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic

cmake --build build-backend-windows
```

### 7) Run backend

```powershell
.\build-backend-windows\backend_app.exe
```

---

## Config discovery order

At runtime the backend searches config in this order:

1. `--config <path>`
2. environment variable `PM_CONFIG`
3. `./config/config.json` next to the binary
4. `./config/config.json` from the current working directory
5. `./backend/config/config.json` from the repo root

---

## Recommended dev flow

1. Start Postgres
2. Apply migrations
3. Build backend
4. Run backend
5. Register a user
6. Promote one dev user to `role = admin`
7. Use admin Bearer token for market creation / deposits / lifecycle actions
8. Run the integration script:
   ```bash
   ./scripts/check_all.sh --deep
   ```

---

## Handy auth smoke commands

### Register

```bash
curl -k https://localhost:8443/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "email":"user@example.com",
    "username":"user1",
    "password":"password123"
  }'
```

### Login

```bash
curl -k https://localhost:8443/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "login":"user@example.com",
    "password":"password123"
  }'
```

### Me

```bash
curl -k https://localhost:8443/me \
  -H "Authorization: Bearer <access_token>"
```

See `docs/api.md` for the full endpoint reference.
