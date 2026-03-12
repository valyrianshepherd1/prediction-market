#!/usr/bin/env bash
set -euo pipefail

CONTAINER=${1:-pm_postgres}
USER=${POSTGRES_USER:-pm}
DB=${POSTGRES_DB:-pmdb}

for f in db/migrations/*.sql; do
  echo "Applying $f..."
  docker exec -i "$CONTAINER" psql -U "$USER" -d "$DB" < "$f"
done

echo "Done."