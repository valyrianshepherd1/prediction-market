$ErrorActionPreference = "Stop"

$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$DeployDir = Join-Path $RootDir "deploy"
$EnvFile = Join-Path $DeployDir ".env"
$EnvExample = Join-Path $DeployDir ".env.example"

if (-not (Test-Path $EnvFile)) {
    Write-Host "[dev_db_up] $EnvFile not found, copying from .env.example"
    Copy-Item $EnvExample $EnvFile
}

Write-Host "[dev_db_up] Starting Postgres via docker compose..."
Push-Location $DeployDir
docker compose up -d
Pop-Location

Write-Host "[dev_db_up] Waiting for Postgres to become healthy..."
$healthy = $false
for ($i=0; $i -lt 60; $i++) {
    try {
        $status = docker inspect -f '{{.State.Health.Status}}' pm_postgres
        if ($status -eq "healthy") { $healthy = $true; break }
    } catch {}
    Start-Sleep -Seconds 1
}

if (-not $healthy) {
    Write-Host "[dev_db_up] Postgres did not become healthy in time."
    docker logs pm_postgres --tail 200
    exit 1
}

# загрузим переменные из .env в текущую сессию (для db_migrate.sh)
Get-Content $EnvFile | ForEach-Object {
    if ($_ -match '^\s*#') { return }
    if ($_ -match '^\s*$') { return }
    $pair = $_.Split("=",2)
    if ($pair.Length -eq 2) {
        [System.Environment]::SetEnvironmentVariable($pair[0], $pair[1], "Process")
    }
}

Write-Host "[dev_db_up] Applying migrations..."
bash (Join-Path $RootDir "scripts\db_migrate.sh") pm_postgres

Write-Host "[dev_db_up] Done. Postgres is up and migrations applied."