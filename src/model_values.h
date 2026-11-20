/* model_values.h — the hardware values that must match the uTracer model.
 *
 * Part of uTmax6, a fork of uTmax by Nick Barton. GNU GPL v3 or later.
 *
 * uTmax converts volts and amps to the uTracer's numbers with four values from
 * cal.txt (the calibration form): the bottom resistor of the anode/screen
 * divider (RaVal), the two current-sense resistors (IaRsense, IsRsense) and the
 * grid full scale (VgMax). They differ between the models:
 *
 *                 RaVal    Rsense   VgMax
 *     uTracer3    6800 Ω   18 Ω     50 V    (uTmax 3.01 defaults; Dekker uTracer3
 *                                            log §15: 6k81, 18 Ω, -50 V)
 *     uTracer6    4700 Ω   4.7 Ω   100 V    (Dekker, uTracer6 log: §8 R1 = 4k7,
 *                                            §10 Rs = 4.7 Ω, §9 n = 32767 |Vg| / 100)
 *
 * In uTmax 3.01, ticking "uTracer6" set VgMax to 100 and left the other three at
 * the uTracer3's values (and unticking it set VgMax to 100 as well). With the
 * divider at 6800 Ω, a request for 300 V charged a uTracer6's reservoir to about
 * 460 V while the screen showed 299 V, and the current shown was a quarter of the
 * real one (tests/test_model_values.py). Its "Reset calibration" button set
 * 5350 Ω / 5.1 Ω / 50 V, which fits neither model.
 *
 * uTmax6:
 *   - switchModelValues(): changing the model replaces each value that is still
 *     at the other model's default with this model's default. Values the user
 *     has changed are left alone.
 *   - modelDefaults(): what "Reset calibration" sets.
 *   - modelValuesProblem(): Start refuses a uTracer6 whose divider or grid full
 *     scale cannot be right (more than 10 % from Dekker's value). The sense
 *     resistors are not checked: Dekker lets the user fit other values (§10).
 *     The uTracer3 is not checked: modified uTracer3s with other dividers and
 *     sense resistors exist, and too low a divider value on it errs on the safe
 *     side (lower voltage than asked).
 *
 * Tested in tests/cpp/test_model_values.cpp and, end to end, in
 * tests/test_model_values.py.
 */
#ifndef MODEL_VALUES_H
#define MODEL_VALUES_H

#include <cmath>
#include <cstdio>
#include <string>

struct ModelValues {
    float RaVal;      // divider bottom resistor, ohms
    float IaRsense;   // anode current-sense resistor, ohms
    float IsRsense;   // screen current-sense resistor, ohms
    float VgMax;      // grid full scale, volts
};

inline ModelValues modelDefaults(bool uTracer6)
{
    if (uTracer6)
        return {4700.0f, 4.7f, 4.7f, 100.0f};
    return {6800.0f, 18.0f, 18.0f, 50.0f};
}

inline bool sameValue(float a, float b)
{
    return std::fabs(a - b) <= 1e-3f * std::fabs(b);
}

// The model has just been set to uTracer6 (or back to uTracer3). Each value
// still at the other model's default becomes this model's default. Returns
// true if anything changed.
inline bool switchModelValues(ModelValues &v, bool uTracer6)
{
    const ModelValues from = modelDefaults(!uTracer6);
    const ModelValues to = modelDefaults(uTracer6);
    bool changed = false;
    auto swap = [&](float &x, float old, float now) {
        if (sameValue(x, old)) { x = now; changed = true; }
    };
    swap(v.RaVal, from.RaVal, to.RaVal);
    swap(v.IaRsense, from.IaRsense, to.IaRsense);
    swap(v.IsRsense, from.IsRsense, to.IsRsense);
    swap(v.VgMax, from.VgMax, to.VgMax);
    return changed;
}

// Empty if the values can be right for the model; otherwise what is wrong, for
// the message at Start.
inline std::string modelValuesProblem(const ModelValues &v, bool uTracer6)
{
    if (!uTracer6)
        return "";
    const ModelValues d = modelDefaults(true);
    std::string out;
    auto check = [&](float x, float want, const char *what, const char *unit) {
        if (!(std::fabs(x - want) <= 0.10f * want)) {   // also catches NaN
            char line[200];
            std::snprintf(line, sizeof line, "%s is %g %s; a uTracer6 has %g %s.\n",
                          what, x, unit, want, unit);
            out += line;
        }
    };
    check(v.RaVal, d.RaVal, "The divider resistor (RaVal)", "ohm");
    check(v.VgMax, d.VgMax, "The grid full scale (VgMax)", "V");
    return out;
}

#endif
