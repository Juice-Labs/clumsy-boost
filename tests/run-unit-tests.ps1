# Compiles and runs the pure-logic unit tests in tests/test_timing.c.
#
# These have no Windows/IUP/WinDivert dependencies, so they build in isolation
# with just the MSVC C compiler and run on any dev machine (no Admin, no driver,
# no network). Returns a non-zero exit code if any test fails, so it can gate CI
# or a pre-commit check.
#
# Usage:  powershell -ExecutionPolicy Bypass -File tests\run-unit-tests.ps1

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$testSrc  = Join-Path $repoRoot "tests\test_timing.c"
$outExe   = Join-Path $env:TEMP "clumsy_test_timing.exe"

# Locate the VS2019 Build Tools C/C++ compiler environment.
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    # fall back to Community edition if BuildTools isn't present
    $vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
}
if (-not (Test-Path $vcvars)) {
    Write-Error "Could not find vcvars64.bat. Install VS2019 Build Tools or adjust this script."
    exit 2
}

$outObj = Join-Path $env:TEMP "clumsy_test_timing.obj"

Write-Host "Compiling $testSrc ..."
# /W4 for strict warnings on the tested code; /WX so warnings fail the build.
# /Fo and /Fe direct the .obj and .exe to TEMP so no build artifacts land in the repo.
cmd /c "`"$vcvars`" >nul 2>&1 && cl /nologo /W4 /WX /Fo:`"$outObj`" /Fe:`"$outExe`" `"$testSrc`" >nul"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Compilation failed."
    exit $LASTEXITCODE
}

Write-Host "Running tests...`n"
& $outExe
$testExit = $LASTEXITCODE

Remove-Item $outExe -ErrorAction SilentlyContinue
exit $testExit
