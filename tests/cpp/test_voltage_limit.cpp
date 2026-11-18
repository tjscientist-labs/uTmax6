// Tests for src/voltage_limit.h: the anode/screen backstop before each measurement.
// Built and run by tests/test_voltage_limit.py, or by hand:
//   g++ -std=c++17 -Wall -o /tmp/tvl tests/cpp/test_voltage_limit.cpp && /tmp/tvl
// Exit status 0 = all passed.
#include "../../src/voltage_limit.h"
#include <cmath>
#include <cstdio>

static int failures = 0;

static void check(bool ok, const char *what) {
    if (!ok) { std::printf("FAIL: %s\n", what); failures++; }
}

int main() {
    const bool U3 = false, U6 = true;

    // uTracer6: the bug. 3.01 stopped a delayed sweep at anything above 425 V.
    check(hvWithinLimit(500, 0, U6, 1000), "uTracer6, VaMax 1000: 500 V allowed");
    check(hvWithinLimit(1000, 1000, U6, 1000), "uTracer6, VaMax 1000: 1000 V allowed");
    check(hvWithinLimit(1000.4f, 0, U6, 1000), "float slack: 1000.4 V allowed");
    check(!hvWithinLimit(1002, 0, U6, 1000), "uTracer6: 1002 V refused");
    check(!hvWithinLimit(0, 1002, U6, 1000), "uTracer6: screen 1002 V refused");

    // The user's maximum is honoured below the hardware ceiling
    check(hvWithinLimit(300, 300, U6, 300), "uTracer6, VaMax 300: 300 V allowed");
    check(!hvWithinLimit(450, 0, U6, 300), "uTracer6, VaMax 300: 450 V refused");

    // ...and capped by it above
    check(!hvWithinLimit(1500, 0, U6, 2000), "uTracer6, VaMax 2000: 1500 V refused");
    check(!hvWithinLimit(500, 0, U3, 1000), "uTracer3, VaMax 1000: 500 V refused");

    // uTracer3 keeps 3.01's behaviour: up to 425 V
    check(hvWithinLimit(400, 400, U3, 400), "uTracer3, VaMax 400: 400 V allowed");
    check(hvWithinLimit(425, 0, U3, 1000), "uTracer3: 425 V allowed (3.01's figure)");
    check(!hvWithinLimit(427, 0, U3, 1000), "uTracer3: 427 V refused");

    // VaMax not set: the hardware ceiling alone
    check(hvWithinLimit(900, 0, U6, 0), "uTracer6, VaMax 0: 900 V allowed");
    check(hvWithinLimit(900, 0, U6, NAN), "uTracer6, VaMax NaN: 900 V allowed");
    check(!hvWithinLimit(1100, 0, U6, 0), "uTracer6, VaMax 0: 1100 V refused");

    // A NaN voltage is never sent
    check(!hvWithinLimit(NAN, 0, U6, 1000), "NaN anode refused");
    check(!hvWithinLimit(0, NAN, U6, 1000), "NaN screen refused");

    if (failures == 0) std::printf("all voltage-limit checks passed\n");
    return failures == 0 ? 0 : 1;
}
