param(
    [Parameter(Position = 0)]
    [string]$Version,

    [ValidateSet("major", "minor", "patch", "build")]
    [string]$Bump = "build",

    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Parse-Version {
    param([string]$Value)
    if ($Value -notmatch '^\d+\.\d+\.\d+\.\d+$') {
        throw "Invalid version format: '$Value'. Use MAJOR.MINOR.PATCH.BUILD (example: 0.9.1.1)."
    }
    return ($Value -split '\.') | ForEach-Object { [int]$_ }
}

function Join-Version {
    param([int[]]$Parts)
    return ($Parts -join '.')
}

function Increment-Version {
    param(
        [int[]]$Parts,
        [string]$Level
    )

    switch ($Level) {
        "major" { $Parts[0]++; $Parts[1] = 0; $Parts[2] = 0; $Parts[3] = 0; break }
        "minor" { $Parts[1]++; $Parts[2] = 0; $Parts[3] = 0; break }
        "patch" { $Parts[2]++; $Parts[3] = 0; break }
        "build" { $Parts[3]++; break }
        default { throw "Unsupported bump level: $Level" }
    }

    return $Parts
}

function Update-FileContent {
    param(
        [string]$Path,
        [scriptblock]$Transform
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "File not found: $Path"
    }

    $reader = [System.IO.StreamReader]::new($Path, $true)
    try {
        $original = $reader.ReadToEnd()
        $encoding = $reader.CurrentEncoding
    } finally {
        $reader.Dispose()
    }
    $updated = & $Transform $original

    if ($original -ne $updated) {
        if ($DryRun) {
            Write-Host "[DryRun] Would update: $Path"
        } else {
            [System.IO.File]::WriteAllText($Path, $updated, $encoding)
            Write-Host "Updated: $Path"
        }
    } else {
        Write-Host "No changes needed: $Path"
    }
}

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

$versionHeader = Join-Path $repoRoot "src\apps\novadesk\Version.h"
if (-not (Test-Path -LiteralPath $versionHeader)) {
    throw "Could not locate Version.h at expected path: $versionHeader"
}

$versionHeaderText = Get-Content -LiteralPath $versionHeader -Raw
$currentVersionMatch = [regex]::Match($versionHeaderText, '#define\s+NOVADESK_VERSION\s+"(?<v>\d+\.\d+\.\d+\.\d+)"')
if (-not $currentVersionMatch.Success) {
    throw "Could not parse current version from Version.h"
}

$currentVersion = $currentVersionMatch.Groups["v"].Value
$targetVersion = $Version

if ([string]::IsNullOrWhiteSpace($targetVersion)) {
    $parts = Parse-Version $currentVersion
    $nextParts = Increment-Version $parts $Bump
    $targetVersion = Join-Version $nextParts
}

[void](Parse-Version $targetVersion)

$targetVersionComma = $targetVersion -replace '\.', ','
Write-Host "Current version: $currentVersion"
Write-Host "Target version : $targetVersion"
if ($DryRun) {
    Write-Host "Dry run mode   : ON"
}

$novadeskRc = Join-Path $repoRoot "src\apps\novadesk\Novadesk.rc"
$manageMain = Join-Path $repoRoot "src\apps\manage_novadesk\main.cpp"
$manageRc = Join-Path $repoRoot "src\apps\manage_novadesk\manage_novadesk.rc"
$setupNsi = Join-Path $repoRoot "installer\setup.nsi"

Update-FileContent -Path $versionHeader -Transform {
    param($text)
    [regex]::Replace(
        $text,
        '(?m)^(#define\s+NOVADESK_VERSION\s+)"\d+\.\d+\.\d+\.\d+"',
        ('${1}"' + $targetVersion + '"'),
        1
    )
}

Update-FileContent -Path $novadeskRc -Transform {
    param($text)
    $text = [regex]::Replace($text, '(?m)^(\s*FILEVERSION\s+)\d+,\d+,\d+,\d+', ('${1}' + $targetVersionComma), 1)
    $text = [regex]::Replace($text, '(?m)^(\s*PRODUCTVERSION\s+)\d+,\d+,\d+,\d+', ('${1}' + $targetVersionComma), 1)
    $text = [regex]::Replace($text, '(?m)^(\s*VALUE\s+"FileVersion",\s*)"\d+\.\d+\.\d+\.\d+"', ('${1}"' + $targetVersion + '"'), 1)
    $text = [regex]::Replace($text, '(?m)^(\s*VALUE\s+"ProductVersion",\s*)"\d+\.\d+\.\d+\.\d+"', ('${1}"' + $targetVersion + '"'), 1)
    return $text
}

Update-FileContent -Path $manageMain -Transform {
    param($text)
    $text = [regex]::Replace($text, '(?m)^(static\s+const\s+wchar_t\s+\*kCurrentVersion\s*=\s*L")\d+\.\d+\.\d+\.\d+(";\s*)$', ('${1}' + $targetVersion + '${2}'), 1)
    $text = [regex]::Replace($text, '(?m)^(\s*g_aboutVersion\s*=\s*CreateWindowExW\([^,\r\n]*,\s*L"STATIC",\s*L"Version\s+)\d+\.\d+\.\d+\.\d+(\s*\(Beta\)".*)$', ('${1}' + $targetVersion + '${2}'), 1)
    return $text
}

Update-FileContent -Path $manageRc -Transform {
    param($text)
    $text = [regex]::Replace($text, '(?m)^(\s*FILEVERSION\s+)\d+,\d+,\d+,\d+', ('${1}' + $targetVersionComma), 1)
    $text = [regex]::Replace($text, '(?m)^(\s*PRODUCTVERSION\s+)\d+,\d+,\d+,\d+', ('${1}' + $targetVersionComma), 1)
    $text = [regex]::Replace($text, '(?m)^(\s*VALUE\s+"FileVersion",\s*)"\d+\.\d+\.\d+\.\d+"', ('${1}"' + $targetVersion + '"'), 1)
    $text = [regex]::Replace($text, '(?m)^(\s*VALUE\s+"ProductVersion",\s*)"\d+\.\d+\.\d+\.\d+"', ('${1}"' + $targetVersion + '"'), 1)
    return $text
}

Update-FileContent -Path $setupNsi -Transform {
    param($text)
    [regex]::Replace(
        $text,
        '(?m)^(!define\s+VERSION\s+)"\d+\.\d+\.\d+\.\d+"',
        ('${1}"' + $targetVersion + '"'),
        1
    )
}

Write-Host "Version bump completed."
