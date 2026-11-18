/* grid_word.h — the uTracer6 grid word, as Ronald Dekker's firmware reads it.
 *
 * Part of uTmax6, a fork of uTmax by Nick Barton. GNU GPL v3 or later.
 *
 * Dekker, uTracer6 construction log, section 9 ("DAC control"),
 * https://www.dos4ever.com/uTracerlog6/tubetester6.html :
 *
 *     n = 32767 * |Vg| / 100;   if n > 32767 then n = 32767
 *     if Vg > 0.0001 then n = n OR 8000H
 *
 * so bits 0-14 hold the magnitude (100 V full scale) and bit 15 is set for a
 * POSITIVE grid (positive voltages need Dekker's positive-grid extension board).
 * The firmware sends bits 0-14 to DAC A for a negative grid and to DAC B for a
 * positive one.
 *
 * Why this is a separate function: uTmax 3.01 computed a signed number here and
 * returned it as unsigned. For a positive grid the number was negative, "%04X"
 * printed it as eight hex digits, and the 18-character command was cut short:
 * +1 V went out as grid word FFFF (+100 V to the firmware) and the heater word
 * was lost. Keeping the encoding in one small function lets it be tested on its
 * own (tests/cpp/test_grid_word.cpp) and guarantees a 16-bit result.
 */
#ifndef GRID_WORD_H
#define GRID_WORD_H

#include <cmath>
#include <cstdint>

// volts:     the grid voltage to send, after calibration (negative = normal bias)
// fullScale: the grid full scale in volts (calData.VgMax; 100 on a uTracer6)
// Returns the 16-bit word for the command, always 0000-FFFF.
inline uint16_t utracer6GridWord(float volts, float fullScale)
{
    if (!(fullScale > 0.0f))                 // guards against 0 or NaN in cal.txt
        return 0;
    long n = std::lround(32767.0 * std::fabs(volts) / fullScale);
    if (n > 32767)
        n = 32767;
    uint16_t word = uint16_t(n);
    if (volts > 0.0001f)
        word |= 0x8000;                      // positive grid
    return word;
}

#endif // GRID_WORD_H
