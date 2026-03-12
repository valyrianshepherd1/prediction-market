#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CERT_DIR="${ROOT_DIR}/backend/config/certs"

mkdir -p "${CERT_DIR}"

CRT="${CERT_DIR}/server.crt"
KEY="${CERT_DIR}/server.key"

if [[ -f "${CRT}" && -f "${KEY}" ]]; then
  echo "[gen_dev_certs] Cert already exists:"
  echo "  ${CRT}"
  echo "  ${KEY}"
  echo "Use: rm -f \"${CRT}\" \"${KEY}\" to regenerate."
  exit 0
fi

command -v openssl >/dev/null 2>&1 || {
  echo "openssl not found. Install it (brew install openssl) or use system openssl."
  exit 1
}

openssl req -x509 -newkey rsa:4096 -sha256 -nodes \
  -keyout "${KEY}" -out "${CRT}" -days 365 \
  -subj "/CN=localhost"

echo "[gen_dev_certs] Generated:"
echo "  ${CRT}"
echo "  ${KEY}"
echo "Now build in CLion, CMake will copy backend/config -> <bin_dir>/config"