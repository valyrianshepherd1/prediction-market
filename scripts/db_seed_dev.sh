#!/usr/bin/env bash
set -euo pipefail

# Repo root
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

CONTAINER="${1:-pm_postgres}"
USER="${POSTGRES_USER:-pm}"
DB="${POSTGRES_DB:-pmdb}"

SEED_FILE="$ROOT_DIR/db/seeds/dev_markets.sql"

if [[ ! -f "$SEED_FILE" ]]; then
  echo "Seed file not found: $SEED_FILE" >&2
  exit 1
fi

echo "Seeding dev markets into $DB (container: $CONTAINER)..."
docker exec -i "$CONTAINER" psql -U "$USER" -d "$DB" < "$SEED_FILE"
echo "Done."