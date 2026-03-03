#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPLOY_DIR="${ROOT_DIR}/deploy"
ENV_FILE="${DEPLOY_DIR}/.env"
ENV_EXAMPLE="${DEPLOY_DIR}/.env.example"

if [[ ! -f "${ENV_FILE}" ]]; then
echo "[dev_db_up] ${ENV_FILE} not found, copying from .env.example"
cp "${ENV_EXAMPLE}" "${ENV_FILE}"
fi

echo "[dev_db_up] Starting Postgres via docker compose..."
pushd "${DEPLOY_DIR}" >/dev/null
docker compose up -d
popd >/dev/null

echo "[dev_db_up] Waiting for Postgres to become healthy..."
for i in {1..60}; do
status="$(docker inspect -f '{{.State.Health.Status}}' pm_postgres 2>/dev/null || true)"
if [[ "${status}" == "healthy" ]]; then
echo "[dev_db_up] Postgres is healthy."
break
fi
sleep 1
done

if [[ "$(docker inspect -f '{{.State.Health.Status}}' pm_postgres 2>/dev/null || true)" != "healthy" ]]; then
echo "[dev_db_up] Postgres did not become healthy in time."
docker logs pm_postgres --tail 200 || true
exit 1
fi

# Экспортируем переменные для scripts/db_migrate.sh
set -a
# shellcheck disable=SC1090
source "${ENV_FILE}"
set +a

echo "[dev_db_up] Applying migrations..."
"${ROOT_DIR}/scripts/db_migrate.sh" pm_postgres

echo "[dev_db_up] Done. Postgres is up and migrations applied."

#DO MANUALLY chmod +x scripts/dev_db_up.sh