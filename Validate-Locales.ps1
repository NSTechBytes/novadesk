<#
  Validates localization catalogs against locales/en.json.
  Run this before merging any UI string change:
    .\Validate-Locales.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$localeDirectory = Join-Path $root 'locales'
$englishPath = Join-Path $localeDirectory 'en.json'

function Read-JsonObject([string]$path) {
  try {
    $value = Get-Content -Raw -Encoding utf8 -LiteralPath $path | ConvertFrom-Json
    if ($null -eq $value) { throw 'The root value is empty.' }
    return $value
  } catch {
    throw "Invalid JSON in ${path}: $($_.Exception.Message)"
  }
}

if (-not (Test-Path -LiteralPath $englishPath)) {
  throw "English catalog is missing: $englishPath"
}

$english = Read-JsonObject $englishPath
$requiredKeys = @($english.PSObject.Properties.Name | Sort-Object)
if ($requiredKeys.Count -eq 0) { throw 'English catalog has no strings.' }

$failures = [System.Collections.Generic.List[string]]::new()
$catalogs = @(Get-ChildItem -LiteralPath $localeDirectory -Filter '*.json' -File | Sort-Object Name)
foreach ($catalogPath in $catalogs) {
  try { $catalog = Read-JsonObject $catalogPath.FullName }
  catch { $failures.Add($_.Exception.Message); continue }

  $keys = @($catalog.PSObject.Properties.Name)
  $missing = @($requiredKeys | Where-Object { $_ -notin $keys })
  if ($missing.Count) {
    $failures.Add("$($catalogPath.Name): missing keys: $($missing -join ', ')")
  }
  foreach ($key in $requiredKeys) {
    $property = $catalog.PSObject.Properties[$key]
    if ($null -eq $property -or $property.Value -isnot [string] -or
        [string]::IsNullOrWhiteSpace($property.Value)) {
      $failures.Add("$($catalogPath.Name): '$key' must be a non-empty string")
    }
  }
}

if ($failures.Count) {
  $failures | ForEach-Object { Write-Error $_ }
  exit 1
}

Write-Host "Validated $($requiredKeys.Count) strings in $($catalogs.Count) locale catalogs." -ForegroundColor Green
