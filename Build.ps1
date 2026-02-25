param(
    [string]$BuildDir = "build-mingw",
    [string]$Configuration = "Release",
    [string]$Platform = "x64",
    [switch]$Reconfigure
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

function Resolve-CMake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $candidates = @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe",
        "C:\msys64\mingw64\bin\cmake.exe",
        "C:\msys64\ucrt64\bin\cmake.exe",
        "C:\msys64\clang64\bin\cmake.exe"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    throw "cmake.exe was not found in PATH or common install paths. Install CMake or add it to PATH."
}

function Resolve-MingwTool {
    param(
        [string]$ExeName
    )

    $cmd = Get-Command $ExeName -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $candidates = @(
        "C:\msys64\mingw64\bin\$ExeName",
        "C:\msys64\ucrt64\bin\$ExeName",
        "C:\msys64\clang64\bin\$ExeName"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    throw "$ExeName was not found in PATH or common MinGW locations."
}

function Resolve-MSBuild {
    $cmd = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
        if ($found) {
            return $found
        }
    }

    throw "msbuild.exe was not found in PATH and could not be located with vswhere."
}

function Build-Solution {
    param(
        [string]$MSBuildPath,
        [string]$SolutionPath,
        [string]$Config,
        [string]$Plat
    )

    if (-not (Test-Path $SolutionPath)) {
        throw "Solution file not found: $SolutionPath"
    }

    Write-Host "Building $SolutionPath ($Config|$Plat) with MSBuild..." -ForegroundColor Cyan
    & $MSBuildPath $SolutionPath /t:Build /m /nologo /p:Configuration=$Config /p:Platform=$Plat
    if ($LASTEXITCODE -ne 0) {
        throw "MSBuild failed for $SolutionPath ($Config|$Plat)"
    }
}

try {
    $cmake = Resolve-CMake
    $msbuild = Resolve-MSBuild
    $mingwMake = Resolve-MingwTool -ExeName "mingw32-make.exe"
    $mingwCC = Resolve-MingwTool -ExeName "gcc.exe"
    $mingwCXX = Resolve-MingwTool -ExeName "g++.exe"
    $mingwBin = Split-Path -Parent $mingwCC

    Set-Location $RepoRoot
    $env:PATH = "$mingwBin;$env:PATH"

    $installerSln = Join-Path $RepoRoot "src\apps\installer_stub\installer_stub.sln"
    $nwmSln = Join-Path $RepoRoot "src\apps\nwm\nwm.sln"

    Build-Solution -MSBuildPath $msbuild -SolutionPath $installerSln -Config $Configuration -Plat $Platform
    Build-Solution -MSBuildPath $msbuild -SolutionPath $nwmSln -Config $Configuration -Plat $Platform

    $cacheFile = Join-Path $BuildDir "CMakeCache.txt"
    if ($Reconfigure -or -not (Test-Path $cacheFile)) {
        Write-Host "Configuring MinGW build..." -ForegroundColor Cyan
        & $cmake -S . -B $BuildDir -G "MinGW Makefiles" `
            -DCMAKE_MAKE_PROGRAM="$mingwMake" `
            -DCMAKE_C_COMPILER="$mingwCC" `
            -DCMAKE_CXX_COMPILER="$mingwCXX"
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configure failed."
        }
    } else {
        Write-Host "Using existing CMake configuration in $BuildDir" -ForegroundColor DarkGray
    }

    Write-Host "Building MinGW target(s)..." -ForegroundColor Cyan
    & $cmake --build $BuildDir -j
    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed."
    }

    Write-Host "Build completed successfully." -ForegroundColor Green
}
catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
