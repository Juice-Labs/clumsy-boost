# Building clumsy-boost

Windows-only. Builds `clumsy.exe` plus its runtime dependencies (WinDivert
driver/DLL, IUP DLL) via premake + MSBuild. No IDE required.

## Prerequisites

- **Visual Studio 2019 Build Tools** (or VS2019 with the C++ toolset). MSBuild is
  expected at:
  `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe`
- **premake5 — use alpha16**, not the current beta. The beta removed the legacy
  `configuration()` API this project's `premake5.lua` relies on and will fail with
  `attempt to call a nil value (global 'configuration')`.
  Download `premake-5.0.0-alpha16-windows.zip` from
  <https://github.com/premake/premake-core/releases/tag/v5.0.0-alpha16>,
  extract `premake5.exe` into the repo root. (It is gitignored.)

## 1. Generate the Visual Studio solution

Run once per checkout, and again whenever `premake5.lua` changes:

```
.\premake5.exe vs2019
```

This creates `build/clumsy.sln` (and the `.vcxproj`).

## 2. Compile

64-bit (the primary build):

```
& "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe" build\clumsy.sln /p:Configuration=Release /p:Platform=x64
```

32-bit — note the platform is **`Win32`**, not `x32` (passing `x32` errors with
`MSB4126: invalid solution configuration`):

```
& "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe" build\clumsy.sln /p:Configuration=Release /p:Platform=Win32
```

Use `/p:Configuration=Debug` for a console/unoptimized build.

## Output

- 64-bit: `bin\vs\Release\x64\`
- 32-bit: `bin\vs\Release\x32\`

Each folder gets `clumsy.exe` plus `WinDivert.dll`, the WinDivert `.sys` driver(s),
`iup.dll`, and `config.txt` — copied next to the exe automatically by the post-build
step. **Ship the whole folder**, not just the exe. Clumsy must run **as
Administrator** (WinDivert loads a kernel driver).

## Common issues

- **`LNK1104: cannot open file ...clumsy.exe`** — a previous `clumsy.exe` is still
  running and holding the file. Close it, then rebuild.
- **premake `nil value (global 'configuration')`** — you're using a beta premake.
  Use alpha16 (see Prerequisites).
- **`Release|x32` invalid** — use `Platform=Win32` for the 32-bit build.

## Tests

See `tests/README.md`. Quick reference:

```
powershell -ExecutionPolicy Bypass -File tests\run-unit-tests.ps1
```
