# Building With CMake (Windows)

## Requirements
- CMake 3.20+
- Visual Studio 2022 (MSVC v143 toolset)

## Notes
- `NOVADESK_BUILD_QUICKJS` is automatically:
  - `OFF` on MSVC builds
  - `ON` on MSYS/MinGW builds
- You can still override it manually with `-DNOVADESK_BUILD_QUICKJS=ON|OFF`.

## Configure
```powershell
cmake -S . -B out\build -G "Visual Studio 17 2022" -A x64
```

## Build
```powershell
cmake --build out\build --config Release
```

## MSYS2 / MinGW Build (QuickJS-enabled path)
Use a MinGW shell (for example, `MSYS2 MinGW x64`) and run:

```bash
cmake -S . -B out/build-msys -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DNOVADESK_BUILD_QUICKJS=ON
cmake --build out/build-msys --target nwm -j1
cmake --build out/build-msys --target installer_stub -j1
cmake --build out/build-msys --target novadesk -j1
```

## Targets
- `novadesk`
- `nwm`
- `installer_stub`
