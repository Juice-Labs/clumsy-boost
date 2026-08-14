// Pure, dependency-free helpers for lag timing math.
//
// Deliberately free of Windows / IUP / WinDivert headers so the arithmetic that
// governs latency injection can be unit tested standalone (see tests/test_timing.c).
// Keep it that way: only standard C types, no side effects.
#pragma once

// Convert a lag magnitude expressed in 1/unitsPerSecond of a second into
// QueryPerformanceCounter ticks.
//   whole milliseconds       -> unitsPerSecond = 1000
//   hundredths of a millisec -> unitsPerSecond = 100000
// Multiplies in 64-bit to avoid overflow at large lag values / high QPC frequencies.
static inline long long lagValueToQpcTicks(long value, long long qpcFreq, long unitsPerSecond) {
    return (long long)value * qpcFreq / unitsPerSecond;
}

// Returns nonzero if a packet captured at packetTicks is due for release at
// nowTicks given a delay of lagTicks. Preserves the original strict-greater
// comparison (currentTime > timestamp + lag).
static inline int lagIsDue(long long nowTicks, long long packetTicks, long long lagTicks) {
    return nowTicks > packetTicks + lagTicks;
}

// Convert a relative wait in whole milliseconds into the units SetWaitableTimer
// expects: 100-nanosecond intervals, negative to mean relative (not absolute) time.
// 1 ms = 10,000 * 100ns.
static inline long long relativeDueTime100ns(unsigned long ms) {
    return -((long long)ms * 10000);
}
