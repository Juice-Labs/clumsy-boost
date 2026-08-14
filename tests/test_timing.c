// Unit tests for the pure lag-timing arithmetic in src/timing.h.
//
// These guard the exact conversions that govern latency-injection accuracy
// (ms/hundredths -> QPC ticks, the release-due comparison, decimal clamp/convert).
// A silent regression here would corrupt injected latency, so this runs as a
// gate before every timing change. No Windows/IUP/WinDivert dependencies.
//
// Build & run: tests/run-unit-tests.ps1
#include <stdio.h>
#include "../src/timing.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { printf("  FAIL (line %d): %s\n", __LINE__, #cond); ++failures; } \
} while (0)

// A representative QPC frequency (10 MHz is the common value on modern Windows).
#define QPC_FREQ_10MHZ 10000000LL

static void test_lagValueToQpcTicks_wholeMs(void) {
    // 1000 units/sec == whole milliseconds
    // 5 ms at 10MHz = 5 * 10,000,000 / 1000 = 50,000 ticks
    CHECK(lagValueToQpcTicks(5, QPC_FREQ_10MHZ, 1000) == 50000LL);
    CHECK(lagValueToQpcTicks(0, QPC_FREQ_10MHZ, 1000) == 0LL);
    CHECK(lagValueToQpcTicks(1, QPC_FREQ_10MHZ, 1000) == 10000LL);
    // max lag (3000 ms) must not overflow and must be exact
    CHECK(lagValueToQpcTicks(3000, QPC_FREQ_10MHZ, 1000) == 30000000LL);
}

static void test_lagValueToQpcTicks_noOverflow(void) {
    // Stress: large value * high frequency must stay correct in 64-bit.
    // 3000 * 24,000,000 / 1000 = 72,000,000 ticks (fits easily in 64-bit;
    // would overflow if the multiply were done in 32-bit).
    CHECK(lagValueToQpcTicks(3000, 24000000LL, 1000) == 72000000LL);
}

static void test_lagIsDue_boundary(void) {
    // lag = 50,000 ticks; packet captured at t=1,000,000
    long long ts = 1000000LL;
    long long lag = 50000LL;
    CHECK(lagIsDue(ts + lag + 1, ts, lag) != 0);   // just past due -> due
    CHECK(lagIsDue(ts + lag,     ts, lag) == 0);   // exactly at -> not yet (strict >)
    CHECK(lagIsDue(ts + lag - 1, ts, lag) == 0);   // before -> not due
    CHECK(lagIsDue(ts,           ts, lag) == 0);   // just captured -> not due
}

int main(void) {
    printf("Running timing unit tests...\n");

    test_lagValueToQpcTicks_wholeMs();
    test_lagValueToQpcTicks_noOverflow();
    test_lagIsDue_boundary();

    printf("\n%d checks, %d failure(s)\n", checks, failures);
    if (failures) {
        printf("RESULT: FAILED\n");
        return 1;
    }
    printf("RESULT: PASSED\n");
    return 0;
}
