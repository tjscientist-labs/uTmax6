/* voltage_limit.h — the last check before an anode or screen voltage is sent.
 *
 * Part of uTmax6, a fork of uTmax by Nick Barton. GNU GPL v3 or later.
 *
 * The sweep set-up already keeps every point within the user's maximum
 * (calData.VaMax): it refuses start and end values above it and clamps most
 * points to it (mainwindow.cpp, the sweep-list builder). This is the backstop
 * behind that, applied to every measurement just before it is sent. If it
 * trips, something upstream is wrong, and the sweep is stopped.
 *
 * The limit is the user's maximum, but never more than the hardware can do:
 *
 *     uTracer3 / 3+   425 V   (Nick Barton's backstop in uTmax 3.01, kept)
 *     uTracer6       1000 V   (Dekker, uTracer6 log: anode and screen
 *                              "voltage range 2 to 1000 V")
 *
 * plus 1 V of slack, because one sweep path (triode-connected pentode models)
 * adds its steps up in floating point without clamping, and can land a hair
 * above the end value.
 *
 * Why this is a separate file: uTmax 3.01 had the 425 V figure written into
 * the hold path only (sweeps with a delay). On a uTracer6 that stopped every
 * delayed sweep above 425 V with "Internal Error: Va or Vs excessive", while
 * sweeps without a delay had no backstop at all. Tested on its own in
 * tests/cpp/test_voltage_limit.cpp.
 */
#ifndef VOLTAGE_LIMIT_H
#define VOLTAGE_LIMIT_H

constexpr float UTRACER3_HV_CEILING = 425.0f;
constexpr float UTRACER6_HV_CEILING = 1000.0f;
constexpr float HV_LIMIT_SLACK = 1.0f;

// The highest anode or screen voltage that may be sent, in volts.
// vaMax is the user's maximum from cal.txt; 0, negative or NaN means "not set",
// and the hardware ceiling applies alone.
inline float hvLimit(bool uTracer6, float vaMax)
{
    float ceiling = uTracer6 ? UTRACER6_HV_CEILING : UTRACER3_HV_CEILING;
    float limit = (vaMax > 0.0f && vaMax < ceiling) ? vaMax : ceiling;
    return limit + HV_LIMIT_SLACK;
}

// True if both voltages may be sent. A NaN voltage is never within the limit.
inline bool hvWithinLimit(float va, float vs, bool uTracer6, float vaMax)
{
    float limit = hvLimit(uTracer6, vaMax);
    return va <= limit && vs <= limit;
}

#endif // VOLTAGE_LIMIT_H
