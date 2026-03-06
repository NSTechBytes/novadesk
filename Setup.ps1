param(
    [string]$BuildDir = "build-mingw",
    [string]$SetupScript = "installer\setup.nsi"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

function Resolve-Makensis {
    $cmd = Get-Command makensis.exe -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $candidates = @(
        "C:\Program Files (x86)\NSIS\makensis.exe",
        "C:\Program Files\NSIS\makensis.exe"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    throw "makensis.exe was not found in PATH or common NSIS install paths."
}

try {
    Set-Location $RepoRoot

    $buildScript = Join-Path $RepoRoot "Build.ps1"
    $setupScriptPath = Join-Path $RepoRoot $SetupScript
    $distDir = Join-Path $RepoRoot "dist"
    $setupOutputDir = Join-Path $RepoRoot "installer\dist_output"
    $makensis = Resolve-Makensis

    if (-not (Test-Path $buildScript)) {
        throw "Build.ps1 not found: $buildScript"
    }

    if (-not (Test-Path $setupScriptPath)) {
        throw "NSIS script not found: $setupScriptPath"
    }

    if (Test-Path $distDir) {
        Write-Host "Cleaning dist..." -ForegroundColor Cyan
        Remove-Item -Recurse -Force $distDir
    }

    New-Item -ItemType Directory -Path $setupOutputDir -Force | Out-Null

    Write-Host "Running release build..." -ForegroundColor Cyan
    & powershell -ExecutionPolicy Bypass -File $buildScript -Configuration Release -BuildDir $BuildDir
    if ($LASTEXITCODE -ne 0) {
        throw "Build.ps1 failed."
    }

    Write-Host "Running NSIS setup script..." -ForegroundColor Cyan
    & $makensis $setupScriptPath
    if ($LASTEXITCODE -ne 0) {
        throw "makensis failed."
    }

    Write-Host "Setup completed successfully." -ForegroundColor Green
}
catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
