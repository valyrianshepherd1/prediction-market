$ErrorActionPreference = "Stop"

$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$CertDir = Join-Path $RootDir "backend\config\certs"

New-Item -ItemType Directory -Force -Path $CertDir | Out-Null

$Crt = Join-Path $CertDir "server.crt"
$Key = Join-Path $CertDir "server.key"

if ((Test-Path $Crt) -and (Test-Path $Key)) {
    Write-Host "[gen_dev_certs] Cert already exists:"
    Write-Host "  $Crt"
    Write-Host "  $Key"
    Write-Host "Delete them to regenerate."
    exit 0
}

if (-not (Get-Command openssl -ErrorAction SilentlyContinue)) {
    throw "openssl not found. Install OpenSSL (e.g. via Chocolatey) or use Git for Windows OpenSSL."
}

openssl req -x509 -newkey rsa:4096 -sha256 -nodes `
  -keyout "$Key" -out "$Crt" -days 365 `
  -subj "/CN=localhost"

Write-Host "[gen_dev_certs] Generated:"
Write-Host "  $Crt"
Write-Host "  $Key"
Write-Host "Now rebuild, CMake will copy backend/config -> <bin_dir>\config"