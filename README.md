# prediction-market

Project inspired by Polymarket:
- **backend**: C++23 + Drogon + PostgreSQL
- **frontend**: Qt6 desktop client
- **build system**: CMake + Ninja
- **dependencies**: `vcpkg` (submodule in `extern/vcpkg`)

---

## Repository structure

```text
backend/   Drogon HTTPS backend
frontend/  Qt desktop client
db/        SQL migrations and seeds
deploy/    docker-compose for PostgreSQL
scripts/   helper scripts for certificates and database startup
tests/     test targets
```

---

## What runs where

For local development the project uses:
- **PostgreSQL** in Docker
- **backend** on `https://localhost:8443`
- **frontend** as a local desktop app
- **WebSocket** on `wss://localhost:8443/ws`

The backend config in `backend/config/config.json` points to PostgreSQL at `127.0.0.1:5432` with:
- database: `pmdb`
- user: `pm`
- password: `pm_password`

Those values match `deploy/.env.example`.

---

## 1. Clone the repository

```bash
git clone --recurse-submodules https://github.com/valyrianshepherd1/prediction-market.git
cd prediction-market
```

---

## 2. macOS

### Prerequisites

Install:
- Xcode Command Line Tools
- Homebrew
- CMake 3.26+
- Ninja
- Docker Desktop
- OpenSSL
- autoconf
- Qt6

### Install dependencies

```bash
xcode-select --install
brew install cmake ninja autoconf openssl qt
```

### Bootstrap vcpkg

```bash
./extern/vcpkg/bootstrap-vcpkg.sh
```

### Generate local HTTPS certificates

```bash
chmod +x scripts/gen_dev_certs.sh
./scripts/gen_dev_certs.sh
```

This creates:
- `backend/config/certs/server.crt`
- `backend/config/certs/server.key`

### Start PostgreSQL and apply migrations

```bash
chmod +x scripts/dev_db_up.sh
./scripts/dev_db_up.sh
```

### Build the backend

```bash
cmake -S backend -B build-backend-macos -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/extern/vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=arm64-osx

cmake --build build-backend-macos
```

### Run the backend

```bash
./build-backend-macos/backend_app
```

If config auto-discovery does not work:

```bash
./build-backend-macos/backend_app --config "$PWD/backend/config/config.json"
```

### Build the frontend

```bash
cmake -S frontend -B build-frontend-macos -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"

cmake --build build-frontend-macos
```

### Run the frontend

```bash
./build-frontend-macos/frontend_app
```

The frontend uses `https://localhost:8443` by default. If needed, you can override it:

```bash
PM_API_BASE_URL=https://localhost:8443 ./build-frontend-macos/frontend_app
```

---

## 3. Windows

### Prerequisites

Install:
- Git
- CMake 3.26+
- Docker Desktop
- PowerShell
- OpenSSL available in `PATH`
- **MSYS2 MinGW64** with:
  - GCC / G++
  - Ninja
  - Qt6

### Install MSYS2 packages


```bash
pacman -Syu
pacman -S --needed \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-qt6-base \
  mingw-w64-x86_64-qt6-tools \
  mingw-w64-x86_64-qt6-declarative
```

If `openssl.exe` is not available in PowerShell, install OpenSSL separately and add it to `PATH`.

### Bootstrap vcpkg

In PowerShell from the repo root:

```powershell
.\extern\vcpkg\bootstrap-vcpkg.bat
```

### Generate local HTTPS certificates

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\gen_dev_certs.ps1
```

### Start PostgreSQL and apply migrations

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\dev_db_up.ps1
```

### Build the backend

```powershell
cmake -S backend -B build-backend-windows -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE="$pwd\extern\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic `
  -DVCPKG_HOST_TRIPLET=x64-mingw-dynamic `
  -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/x86_64-w64-mingw32-gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/x86_64-w64-mingw32-g++.exe

cmake --build build-backend-windows
```

### Run the backend

```powershell
.\build-backend-windows\backend_app.exe
```

If needed:

```powershell
.\build-backend-windows\backend_app.exe --config "$pwd\backend\config\config.json"
```

### Build the frontend

```powershell
cmake -S frontend -B build-frontend-windows -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_PREFIX_PATH=C:/msys64/mingw64 `
  -DQt6_DIR=C:/msys64/mingw64/lib/cmake/Qt6 `
  -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/x86_64-w64-mingw32-gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/x86_64-w64-mingw32-g++.exe

cmake --build build-frontend-windows
```

### Run the frontend

```powershell
$env:PM_API_BASE_URL = "https://localhost:8443"
.\build-frontend-windows\frontend_app.exe
```

---