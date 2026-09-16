# Empirical latency-injection regression test.
#
# Measures real injected latency end-to-end and asserts it matches what was
# configured, within tolerance. This is the Tier-2 guard (Tier-1 is the pure
# unit tests in run-unit-tests.ps1): it catches timing regressions that only
# appear with the live WinDivert driver and real traffic.
#
# HOW IT WORKS
#   1. Pings the target with Clumsy NOT running -> baseline RTT.
#   2. Launches Clumsy headless (parameterized mode) applying LagMs to both
#      directions, with a --timeout so it self-closes.
#   3. Pings again -> injected RTT.
#   4. Asserts (injected - baseline) is within tolerance of 2 * LagMs
#      (both directions each add LagMs to a round trip).
#
# REQUIREMENTS
#   - Run in an ELEVATED PowerShell (WinDivert needs Administrator).
#   - A reachable, stable target host (LAN host with low, steady RTT is best).
#
# USAGE
#   powershell -ExecutionPolicy Bypass -File tests\measure-latency.ps1 `
#       -TargetHost 192.168.50.2 -LagMs 5
#
#   Optional: -Iterations 20  -ToleranceMs 3  -ClumsyExe <path>  -Filter <expr>
#
# EXIT CODE: 0 if within tolerance, 1 if out of tolerance, 2 on setup error.

param(
    [Parameter(Mandatory=$true)][string]$TargetHost,
    [Parameter(Mandatory=$true)][double]$LagMs,
    [int]$Iterations = 20,
    [double]$ToleranceMs = 3,
    [string]$ClumsyExe = "",
    [string]$Filter = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $ClumsyExe) {
    $ClumsyExe = Join-Path $repoRoot "bin\vs\Release\x64\clumsy.exe"
}
if (-not (Test-Path $ClumsyExe)) {
    Write-Error "clumsy.exe not found at '$ClumsyExe'. Build it first, or pass -ClumsyExe."
    exit 2
}
if (-not $Filter) {
    $Filter = "ip.DstAddr == $TargetHost or ip.SrcAddr == $TargetHost"
}

# Require elevation up front: without it Clumsy silently loads no driver and
# injects nothing, producing a meaningless "0ms added" result.
$isAdmin = ([Security.Principal.WindowsPrincipal] `
    [Security.Principal.WindowsIdentity]::GetCurrent()
).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Error "This test must run in an ELEVATED PowerShell (WinDivert needs Administrator). Open PowerShell via 'Run as administrator' and re-run."
    exit 2
}

function Get-PingAvgMs([string]$targetHost, [int]$count) {
    # Shell out to ping.exe and average the per-reply "time=Xms" values. This
    # matches the manual measurement methodology and avoids the ~1ms WMI overhead
    # that Test-Connection adds on Windows PowerShell 5.1.
    # NOTE: parses English-locale ping output ("time=Xms" / "time<1ms").
    $output = & ping.exe -n $count $targetHost 2>&1
    $times = @()
    foreach ($line in $output) {
        if ($line -match 'time<1ms') {
            $times += 0            # sub-millisecond reply -> count as 0
        } elseif ($line -match 'time=(\d+)ms') {
            $times += [int]$Matches[1]
        }
    }
    if ($times.Count -eq 0) { throw "No ping replies from $targetHost (host unreachable?)" }
    return ($times | Measure-Object -Average).Average
}

Write-Host "== Empirical latency test =="
Write-Host "Target:     $TargetHost"
Write-Host "Configured: $LagMs ms per direction (expect ~$([math]::Round(2*$LagMs,2)) ms added RTT)"
Write-Host "Tolerance:  +/- $ToleranceMs ms`n"

# --- 1. Baseline (Clumsy off) ---
Write-Host "Measuring baseline RTT (Clumsy off)..."
$baseline = Get-PingAvgMs $TargetHost $Iterations
Write-Host ("  baseline avg = {0:N2} ms`n" -f $baseline)

# --- 2. Launch Clumsy headless with lag on both directions ---
$runSeconds = [math]::Max(15, [int]($Iterations * 1.0) + 8)
# The filter contains spaces, so it MUST be passed as a single double-quoted
# argument -- otherwise Clumsy's arg parser sees "--filter ip.DstAddr" and then
# chokes on the next token. Build the command line explicitly with the filter quoted.
$argString = "--filter `"$Filter`" --lag on --lag-inbound on --lag-outbound on --lag-time $LagMs --timeout $runSeconds"
Write-Host "Launching Clumsy for ~$runSeconds s:`n  $ClumsyExe $argString"
$proc = Start-Process -FilePath $ClumsyExe -ArgumentList $argString -PassThru
Start-Sleep -Seconds 3   # let the driver load and filtering start

try {
    # --- 3. Injected measurement ---
    Write-Host "Measuring injected RTT (Clumsy on)..."
    $injected = Get-PingAvgMs $TargetHost $Iterations
    Write-Host ("  injected avg = {0:N2} ms`n" -f $injected)
}
finally {
    if ($proc -and -not $proc.HasExited) {
        $proc.Kill() | Out-Null
    }
}

# --- 4. Assert ---
$observedDelta = $injected - $baseline
$expectedDelta = 2 * $LagMs
$deviation = [math]::Abs($observedDelta - $expectedDelta)

Write-Host "== Result =="
Write-Host ("  observed added RTT = {0:N2} ms  (expected {1:N2} ms)" -f $observedDelta, $expectedDelta)
Write-Host ("  deviation          = {0:N2} ms  (tolerance {1:N2} ms)" -f $deviation, $ToleranceMs)

# Note: a small positive bias (~1ms) is expected from WinDivert's kernel/user
# round trip; that is why deviation is measured against tolerance, not zero.
if ($deviation -le $ToleranceMs) {
    Write-Host "`nRESULT: PASSED" -ForegroundColor Green
    exit 0
} else {
    Write-Host "`nRESULT: FAILED (injected latency outside tolerance)" -ForegroundColor Red
    exit 1
}
