// Tests for src/grid_word.h: the uTracer6 grid word (Dekker, tubetester6.html §9).
// Built and run by tests/test_grid_word.py, or by hand:
//   g++ -std=c++17 -Wall -o /tmp/tgw tests/cpp/test_grid_word.cpp && /tmp/tgw
// Exit status 0 = all passed. With "--commands" it prints the measure command
// uTmax6 builds for a list of grid voltages, for the Python test to decode.
#include "../../src/grid_word.h"
#include <cstdio>
#include <cstring>

static int failures = 0;

static void expect_word(float volts, uint16_t want) {
    uint16_t got = utracer6GridWord(volts, 100.0f);
    if (got != want) {
        std::printf("FAIL: %+.3f V -> %04X, expected %04X\n", volts, got, want);
        failures++;
    }
}

// The measure command exactly as Sweep_Set() builds it (mainwindow.cpp),
// with the grid word from GetVg(). It must always be 18 characters.
static void command(char *buf, float volts) {
    unsigned VaADC = 0x00E3, VsADC = 0, VfADC = 0x006B;
    unsigned VgADC = utracer6GridWord(volts, 100.0f);
    std::snprintf(buf, 19, "10%04X%04X%04X%04X", VaADC, VsADC, VgADC, VfADC);
}

int main(int argc, char **argv) {
    const float volts[] = {-150.0f, -100.0f, -50.0f, -2.0f, -0.001f, 0.0f,
                           0.001f, 1.0f, 50.0f, 100.0f, 150.0f};
    if (argc > 1 && std::strcmp(argv[1], "--commands") == 0) {
        char buf[32];
        for (float v : volts) { command(buf, v); std::printf("%+.3f %s\n", v, buf); }
        return 0;
    }
    // Dekker's formula: n = 32767 |Vg| / 100, bit 15 set for Vg > 0.
    expect_word(0.0f, 0x0000);
    expect_word(-2.0f, 0x028F);      // 655
    expect_word(-100.0f, 0x7FFF);
    expect_word(-150.0f, 0x7FFF);    // clamped
    expect_word(1.0f, 0x8148);       // 8000H | 328
    expect_word(100.0f, 0xFFFF);
    expect_word(150.0f, 0xFFFF);     // clamped
    expect_word(0.00005f, 0x0000);   // below Dekker's 0.0001 V threshold: no sign bit
    if (utracer6GridWord(-2.0f, 0.0f) != 0) { std::printf("FAIL: zero full scale\n"); failures++; }
    // Every command is exactly 18 characters, whatever the grid voltage.
    char buf[32];
    for (float v : volts) {
        char full[64];
        std::snprintf(full, sizeof full, "10%04X%04X%04X%04X", 0x00E3u, 0u,
                      (unsigned)utracer6GridWord(v, 100.0f), 0x006Bu);
        command(buf, v);
        if (std::strlen(full) != 18 || std::strcmp(full, buf) != 0) {
            std::printf("FAIL: %+.3f V command %s (%zu chars)\n", v, full, std::strlen(full));
            failures++;
        }
    }
    std::printf(failures ? "%d FAILED\n" : "all grid-word tests passed\n", failures);
    return failures ? 1 : 0;
}
