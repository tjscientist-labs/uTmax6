// Tests for src/neg_rail.h: the negative-rail reading in the result string.
// Built and run by tests/test_neg_rail.py, or by hand:
//   g++ -std=c++17 -Wall -o /tmp/tnr tests/cpp/test_neg_rail.cpp && /tmp/tnr
// Exit status 0 = all passed.
#include "../../src/neg_rail.h"
#include <cmath>
#include <cstdio>
#include <initializer_list>

static int failures = 0;

static void near(float got, float want, float tol, const char *what) {
    if (std::fabs(got - want) > tol) {
        std::printf("FAIL: %s: got %.3f, want %.3f\n", what, got, want);
        failures++;
    }
}

int main() {
    const bool U3 = false, U6 = true;

    // uTracer6: Dekker's own worked example, -85 V reads n = 600.
    near(negRailVolts(600, negRailScale(U6)), -85.0f, 0.3f, "uTracer6: n = 600 is -85 V");
    // 3.01 used the -15 V divider (k = 11): n = 600 came out as -17.7 V.
    near(negRailScale(U6), 204.7f / 4.7f, 1e-3f, "uTracer6: k = 200k/4k7 divider");

    // uTracer3: Dekker, V = 0.12 n - 117.5 (k = 24.5, not 3.01's integer 24).
    near(negRailScale(U3), 24.5f, 1e-4f, "uTracer3: k = 24.5");
    for (int n : {600, 646, 800, 1000})
        near(negRailVolts(n, negRailScale(U3)), 0.1197f * n - 117.5f, 0.05f,
             "uTracer3: V = 0.12 n - 117.5");

    // Full scale: code 1023 is +5 V on any divider.
    near(negRailVolts(1023, negRailScale(U6)), 5.0f, 1e-4f, "code 1023 is +5 V");

    if (failures == 0) std::printf("all passed\n");
    return failures == 0 ? 0 : 1;
}
