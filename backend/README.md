# Backend (Drogon + Postgres) — Setup & Run

This folder contains the **HTTPS** Drogon backend (`backend_app`) and its runtime config.

The backend needs:
- Postgres database (dev: Docker Compose)
- `config/config.json` (copied next to the binary on build)
- TLS certs: `config/certs/server.crt` and `config/certs/server.key`

The repo uses **vcpkg** (as a git submodule) to install Drogon with Postgres support.

---

## Quick sanity check (after everything is running)

```bash
curl -k https://localhost:8443/healthz
curl -k https://localhost:8443/healthz/db
```

---

## macOS

### 1) Prerequisites

Install:
- Xcode / Command Line Tools (Apple Clang)
- CMake (>= 3.26) + Ninja
- Docker Desktop (for Postgres dev DB)
- OpenSSL (for generating dev certs)
- autoconf (needed by vcpkg when building libpq on macOS)

Homebrew example:
```bash
brew install cmake ninja autoconf openssl
```

### 2) Clone + init submodules (vcpkg)

```bash
git clone --recurse-submodules https://github.com/valyrianshepherd1/prediction-market.git
cd prediction-market
# if you didn't use --recurse-submodules:
git submodule update --init --recursive
```

### 3) Bootstrap vcpkg

```bash
./extern/vcpkg/bootstrap-vcpkg.sh
```

### 4) Generate dev HTTPS certs

Generates:
- `backend/config/certs/server.crt`
- `backend/config/certs/server.key`

```bash
chmod +x scripts/gen_dev_certs.sh
./scripts/gen_dev_certs.sh
```

### 5) Start Postgres (Docker) + apply migrations

```bash
chmod +x scripts/dev_db_up.sh
./scripts/dev_db_up.sh
```

Optional: seed dev markets
```bash
chmod +x scripts/db_seed_dev.sh
./scripts/db_seed_dev.sh
```

### 6) Build backend (recommended: build backend only)

> Building from the repo root also configures the Qt frontend.  
> For backend-only development, configure **only** the `backend/` folder:

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

Config discovery order:
1) `--config <path>`
2) env `PM_CONFIG`
3) `./config/config.json` next to the binary
4) `./config/config.json` from current working dir
5) `./backend/config/config.json` from repo root

If you want to be explicit:
```bash
./build-backend-macos/backend_app --config "$PWD/backend/config/config.json"
```

### 8) (Optional) Enable admin endpoints

Admin endpoints require header `X-Admin-Token` and this env var:
```bash
export PM_ADMIN_TOKEN="dev_admin_token_123"
```

---

## Windows

### 1) Prerequisites

Install:
- CMake (>= 3.26)
- Ninja
- Git 
- Docker Desktop (for Postgres dev DB)
- OpenSSL (for generating dev certs; Chocolatey or MSYS2)
- PowerShell 

### 2) Clone + init submodules (vcpkg)

PowerShell:
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

Generates:
- `backend\config\certs\server.crt`
- `backend\config\certs\server.key`

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\gen_dev_certs.ps1
```

### 5) Start Postgres (Docker) + apply migrations

Option A: run the helper (may require a `bash` on PATH):
```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\dev_db_up.ps1
```

Option B: do it manually (no bash needed):
```powershell
cd .\deploy
docker compose up -d
cd ..
powershell -ExecutionPolicy Bypass -File .\scripts\db_migrate.ps1
```

Optional: seed dev markets
```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\db_seed_dev.ps1
```

### 6) Build backend (recommended: build backend only)

```powershell
cmake -S backend -B build-backend-windows -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE="$pwd\extern\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic

cmake --build build-backend-windows
```
### 7) Run backend


(folder with binaries)
```powershell
.\cmake-dev-build-debug-macos\backend\backend_app.exe
```


### 8) (Optional) Enable admin endpoints

```powershell
$env:PM_ADMIN_TOKEN = "dev_admin_token_123"
```

---

