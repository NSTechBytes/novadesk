param(
    [string]$BuildDir = "build-mingw",
    [string]$ExeName = "novadesk.exe",
    [switch]$Debug
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildPath = Join-Path $root $BuildDir
$exePath = Join-Path $buildPath $ExeName

if (-not (Test-Path $exePath)) {
    Write-Host "Executable not found: $exePath" -ForegroundColor Red
    exit 1
}

$stdoutLog = Join-Path $buildPath "novadesk.stdout.log"
$stderrLog = Join-Path $buildPath "novadesk.stderr.log"

New-Item -ItemType File -Path $stdoutLog -Force | Out-Null
New-Item -ItemType File -Path $stderrLog -Force | Out-Null

$args = @()
if ($Debug) {
    $args += "--debug"
}

Write-Host "Starting: $exePath $($args -join ' ')" -ForegroundColor Cyan
Write-Host "stdout: $stdoutLog"
Write-Host "stderr: $stderrLog"

$startParams = @{
    FilePath = $exePath
    WorkingDirectory = $buildPath
    NoNewWindow = $true
    PassThru = $true
    RedirectStandardOutput = $stdoutLog
    RedirectStandardError = $stderrLog
}

if ($args.Count -gt 0) {
    $startParams.ArgumentList = $args
}

$proc = Start-Process @startParams

$stdoutTail = Start-Job -ScriptBlock {
    param($path)
    Get-Content -Path $path -Wait
} -ArgumentList $stdoutLog

$stderrTail = Start-Job -ScriptBlock {
    param($path)
    Get-Content -Path $path -Wait
} -ArgumentList $stderrLog

try {
    while (-not $proc.HasExited) {
        Start-Sleep -Milliseconds 200
    }
}
finally {
    Stop-Job $stdoutTail -ErrorAction SilentlyContinue | Out-Null
    Stop-Job $stderrTail -ErrorAction SilentlyContinue | Out-Null
    Remove-Job $stdoutTail -Force -ErrorAction SilentlyContinue | Out-Null
    Remove-Job $stderrTail -Force -ErrorAction SilentlyContinue | Out-Null
}

Write-Host "Process exited with code: $($proc.ExitCode)" -ForegroundColor Yellow
exit $proc.ExitCode
