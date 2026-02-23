# Building Novadesk (Windows + Linux)

This project currently builds a minimal Novadesk runtime app at:

- `src/apps/novadesk/main.cpp`
- `src/Widgets/builtin/main.js`
- `src/Widgets/builtin/meta.json`

The executable reads `meta.json` from the same output directory and loads the `main` entry (for example `main.js`).

## Prerequisites

### Common

- CMake 3.20 or newer
- A C/C++ compiler toolchain

### Windows (MinGW)

- MinGW-w64 (GCC/G++)
- `mingw32-make` (or compatible make)
- Ensure `cmake` and MinGW binaries are on `PATH`

### Linux

- GCC/G++ or Clang
- Make or Ninja
- X11/OpenGL development packages (names vary by distro), typically:
  - `libx11-dev`
  - `libxrandr-dev`
  - `libgl1-mesa-dev`
  - `libpthread-stubs0-dev` (or pthread via libc toolchain)
  - `libdl` (usually provided by glibc toolchain)

## Build

### Windows (MinGW Makefiles)

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw -j
```

Run:

```powershell
.\build-mingw\novadesk.exe
```

### Linux

```bash
cmake -S . -B build-linux
cmake --build build-linux -j
```

Run:

```bash
./build-linux/novadesk
```

## Output Layout

After build, the output folder contains:

- `novadesk` or `novadesk.exe`
- `meta.json` (copied from `src/Widgets/builtin/meta.json`)
- `main.js` (copied from `src/Widgets/builtin/main.js`)
- On MinGW (Windows), runtime DLLs are copied automatically when found:
  - `libstdc++-6.dll`
  - `libwinpthread-1.dll`
  - one of `libgcc_s_seh-1.dll` / `libgcc_s_dw2-1.dll` / `libgcc_s_sjlj-1.dll`

RGFW is compiled in header-only mode (`RGFW_IMPLEMENTATION`) inside the app target, so no separate RGFW runtime library needs to be copied.

`meta.json` example:

```json
{
  "name": "my-novadesk-app",
  "version": "1.0.0",
  "main": "main.js"
}
```

## Troubleshooting

- `cmake: command not found`:
  - Install CMake and add it to `PATH`.
- Linker cannot find RGFW:
  - Ensure `RGFW.h` is available under:
    - `src/third_party/RGFW/win/include` (Windows)
    - `src/third_party/RGFW/linux/include` (Linux)
- App prints `meta.json missing or invalid`:
  - Ensure `meta.json` exists beside the executable and contains `"main"`.
