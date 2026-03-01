# scripts/db_seed_dev.ps1

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

$Container = "pm_postgres"
if ($args.Length -ge 1 -and $args[0]) { $Container = $args[0] }

$User = $env:POSTGRES_USER; if (-not $User) { $User = "pm" }
$Db = $env:POSTGRES_DB; if (-not $Db) { $Db = "pmdb" }

$SeedFile = Join-Path $Root "db\seeds\dev_markets.sql"
if (-not (Test-Path $SeedFile)) {
    Write-Error "Seed file not found: $SeedFile"
    exit 1
}

Write-Host "Seeding dev markets into $Db (container: $Container)..."
Get-Content $SeedFile | docker exec -i $Container psql -U $User -d $Db
Write-Host "Done."