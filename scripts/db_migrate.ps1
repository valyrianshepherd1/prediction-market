$Container = "pm_postgres"
$User = $env:POSTGRES_USER; if (-not $User) { $User = "pm" }
$Db = $env:POSTGRES_DB; if (-not $Db) { $Db = "pmdb" }

Get-ChildItem "db/migrations/*.sql" | ForEach-Object {
  Write-Host "Applying $($_.FullName)..."
  Get-Content $_.FullName | docker exec -i $Container psql -U $User -d $Db
}

Write-Host "Done."