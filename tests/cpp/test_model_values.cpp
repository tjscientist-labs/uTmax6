// Tests for src/model_values.h: hardware values that must match the uTracer model.
// Built and run by tests/test_model_values.py, or by hand:
//   g++ -std=c++17 -Wall -o /tmp/tmv tests/cpp/test_model_values.cpp && /tmp/tmv
// Exit status 0 = all passed.
#include "../../src/model_values.h"
#include <cmath>
#include <cstdio>

static int failures = 0;

static void check(bool ok, const char *what) {
    if (!ok) { std::printf("FAIL: %s\n", what); failures++; }
}

int main() {
    const bool U3 = false, U6 = true;

    // Defaults per model (Dekker's values; uTmax 3.01's for the uTracer3)
    ModelValues d6 = modelDefaults(U6), d3 = modelDefaults(U3);
    check(d6.RaVal == 4700 && d6.IaRsense == 4.7f && d6.IsRsense == 4.7f && d6.VgMax == 100,
          "uTracer6 defaults 4700 / 4.7 / 4.7 / 100");
    check(d3.RaVal == 6800 && d3.IaRsense == 18 && d3.IsRsense == 18 && d3.VgMax == 50,
          "uTracer3 defaults 6800 / 18 / 18 / 50");

    // Ticking uTracer6 on a uTracer3 default set: everything follows
    ModelValues v = d3;
    check(switchModelValues(v, U6), "uTracer3 defaults -> uTracer6: changed");
    check(v.RaVal == 4700 && v.IaRsense == 4.7f && v.IsRsense == 4.7f && v.VgMax == 100,
          "uTracer3 defaults -> uTracer6 defaults");
    // ...and back
    check(switchModelValues(v, U3), "and back: changed");
    check(v.RaVal == 6800 && v.IaRsense == 18 && v.VgMax == 50, "and back to uTracer3 defaults");

    // 3.01's state after ticking: VgMax 100, the rest uTracer3
    v = {6800, 18, 18, 100};
    switchModelValues(v, U6);
    check(v.RaVal == 4700 && v.IaRsense == 4.7f && v.VgMax == 100, "3.01 half-switched state fixed");

    // A value the user changed is left alone
    v = {6750, 10, 18, 50};
    switchModelValues(v, U6);
    check(v.RaVal == 6750, "user's divider kept");
    check(v.IaRsense == 10, "user's anode sense resistor kept");
    check(v.IsRsense == 4.7f, "screen sense resistor at default follows");
    check(v.VgMax == 100, "grid full scale at default follows");
    v = {4700, 4.7f, 4.7f, 100};
    check(!switchModelValues(v, U6), "already uTracer6 values: nothing to change");

    // The check at Start
    check(modelValuesProblem(d6, U6).empty(), "uTracer6 defaults pass");
    check(modelValuesProblem({4800, 18, 18, 100}, U6).empty(), "calibrated divider and other Rs pass");
    check(!modelValuesProblem({6800, 4.7f, 4.7f, 100}, U6).empty(), "uTracer6 with 6800 ohm refused");
    check(!modelValuesProblem({5350, 5.1f, 5.1f, 50}, U6).empty(), "3.01's reset values refused");
    check(!modelValuesProblem({4700, 4.7f, 4.7f, 50}, U6).empty(), "uTracer6 with VgMax 50 refused");
    check(!modelValuesProblem({NAN, 4.7f, 4.7f, 100}, U6).empty(), "NaN divider refused");
    check(modelValuesProblem({6800, 18, 18, 50}, U6).find("RaVal") != std::string::npos,
          "message names RaVal");
    check(modelValuesProblem({6800, 18, 18, 50}, U6).find("VgMax") != std::string::npos,
          "message names VgMax");
    check(modelValuesProblem({5350, 5.1f, 5.1f, 50}, U3).empty(), "uTracer3 not checked");

    if (failures == 0) std::printf("all passed\n");
    return failures == 0 ? 0 : 1;
}
