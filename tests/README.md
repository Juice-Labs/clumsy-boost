# clumsy-boost tests

Two tiers of regression tests guard the latency-injection timing. Run both before
merging any change to the timing path (`src/timing.h`, `src/lag.c`, `src/divert.c`,
`src/utils.c`).

## Tier 1 — unit tests (fast, no Admin/network)

Pure-arithmetic tests over the helpers in `src/timing.h`: ms/hundredths → QPC tick
conversions, the release-due comparison, the waitable-timer due-time conversion, and
the decimal clamp/convert. These are where a silent accuracy regression would hide
(wrong divisor, overflow, truncation).

```
powershell -ExecutionPolicy Bypass -File tests\run-unit-tests.ps1
```

- Compiles `tests/test_timing.c` with the MSVC compiler (VS2019 Build Tools) and runs it.
- No Administrator, no WinDivert driver, no network required — runs on any dev machine.
- Exit code is non-zero if any check fails (suitable as a pre-commit / CI gate).

## Tier 2 — empirical latency test (needs Admin + a target host)

End-to-end measurement with the live WinDivert driver: pings a target with Clumsy
off (baseline), then again with a known lag applied to both directions, and asserts
the added round-trip time matches `2 × LagMs` within tolerance. Catches timing
regressions that only appear under real traffic.

Run in an **elevated** PowerShell (WinDivert needs Administrator):

```
powershell -ExecutionPolicy Bypass -File tests\measure-latency.ps1 -TargetHost 192.168.50.2 -LagMs 5
```

Options: `-Iterations 20`, `-ToleranceMs 3`, `-ClumsyExe <path>`, `-Filter <expr>`.

Notes:
- Uses Clumsy's headless/parameterized mode with `--timeout`, so it self-closes.
- A small positive bias (~1 ms) above the configured value is expected and normal —
  it is the WinDivert kernel↔user round trip, not a regression. That is why the
  assertion uses a tolerance rather than requiring an exact match.
- Best run against a LAN host with low, steady baseline RTT.
