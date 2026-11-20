/* neg_rail.h — the negative supply reading in the uTracer's result string.
 *
 * Part of uTmax6, a fork of uTmax by Nick Barton. GNU GPL v3 or later.
 *
 * The result string carries one ADC word for a negative supply rail. The rail
 * is measured through a divider from the rail up to the +5 V reference, so
 * (Dekker, uTracer6 log, section 8, the "negative boost converter" formula):
 *
 *     Vneg = 5 * (1 - k * (1 - n/1023)),   k = (R_top + R_bottom) / R_bottom
 *
 * Which rail, and which divider:
 *
 *     uTracer3 / 3+   boost converter, 47 kOhm / 2 kOhm   (k = 24.5;
 *                     Dekker's uTracer3 log: V = 0.12 n - 117.5)
 *     uTracer6        the -85 V rail, R27 200 kOhm / R29 4k7 to AN11
 *                     (k = 43.55; Dekker, uTracer6 log, schematic fig. 7.2 and
 *                     section 9: "RB4/AN11 becomes ADC input to measure the
 *                     -85 V output voltage"; formula: -85 V gives n = 600)
 *     https://www.dos4ever.com/uTracerlog6/tubetester6.html
 *
 * Why this is a separate file: uTmax 3.01 used the uTracer6's -15 V divider
 * (47k / 4k7, k = 11) for a reading that comes from the -85 V rail, so a healthy
 * uTracer6 showed about -17.7 V. For the uTracer3 it wrote (47 + 2)/2, which in
 * C++ is integer division (24, not 24.5), so it read about 2 % off. Tested on its
 * own in tests/cpp/test_neg_rail.cpp.
 */
#ifndef NEG_RAIL_H
#define NEG_RAIL_H

constexpr float UTRACER3_NEG_TOP_OHM = 47000.0f;
constexpr float UTRACER3_NEG_BOTTOM_OHM = 2000.0f;
constexpr float UTRACER6_NEG_TOP_OHM = 200000.0f;   // R27, -85 V rail
constexpr float UTRACER6_NEG_BOTTOM_OHM = 4700.0f;  // R29

// The divider ratio k for the negative-rail word.
inline float negRailScale(bool uTracer6)
{
    float top = uTracer6 ? UTRACER6_NEG_TOP_OHM : UTRACER3_NEG_TOP_OHM;
    float bottom = uTracer6 ? UTRACER6_NEG_BOTTOM_OHM : UTRACER3_NEG_BOTTOM_OHM;
    return (top + bottom) / bottom;
}

// The rail in volts from its ADC word (0-1023), before calibration.
inline float negRailVolts(float code, float scale)
{
    return 5.0f * (scale * (code / 1023.0f - 1.0f) + 1.0f);
}

#endif // NEG_RAIL_H
