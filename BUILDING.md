# Building With CMake (Windows)

## Requirements
- CMake 3.20+
- Visual Studio 2022 (MSVC v143 toolset)

## Configure
```powershell
cmake -S . -B out\build -G "Visual Studio 17 2022" -A x64
```

## Build
```powershell
cmake --build out\build --config Release
```

## Targets
- `novadesk`
- `nwm`
- `installer_stub`

