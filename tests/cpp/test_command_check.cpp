// Tests for src/command_check.h: the last check on a command before it is sent.
// Built and run by tests/test_command_check.py, or by hand:
//   g++ -std=c++17 -Wall -o /tmp/tcc tests/cpp/test_command_check.cpp && /tmp/tcc
// Exit status 0 = all passed.
#include "../../src/command_check.h"
#include "../../src/grid_word.h"
#include <cstdio>

static int failures = 0;

static void expect(const char *cmd, CmdProblem want, const CommandLimits &lim, const char *what) {
    CmdProblem got = checkCommand(cmd, lim);
    if (got != want) {
        std::printf("FAIL: %s: %s gave \"%s\", expected \"%s\"\n", what, cmd,
                    problemText(got), problemText(want));
        failures++;
    }
}

int main() {
    CommandLimits u3;                       // uTracer3, 10-bit grid, no limit below full scale
    CommandLimits u3fw203; u3fw203.grid12bit = true;
    CommandLimits u6; u6.uTracer6 = true;
    CommandLimits u6lim; u6lim.uTracer6 = true; u6lim.maxAnodeWord = 0x132; u6lim.maxScreenWord = 0x132;
    CommandLimits ext; ext.extHeater = true;
    using P = CmdProblem;

    // Commands uTmax really sends (from the simulator logs)
    expect("00000000008F400808", P::None, u6, "settings: 8F, auto averaging, auto gains");
    expect("10002F01320148006A", P::None, u6, "measure");
    expect("2000720072014801AA", P::None, u6, "apply and hold");
    expect("300000000000000000", P::None, u6, "end measurement");
    expect("400000000000000056", P::None, u6, "heater");
    expect("500000000000000000", P::None, u6, "read ADC");
    expect("100016001600000000", P::None, u6, "End(): 4 V, heater off");

    // Form
    expect("1000160016000000", P::Length, u6, "16 characters (a cut-short command)");
    expect("10001600160000000000", P::Length, u6, "20 characters");
    expect("10002f01320148006A", P::NotHex, u6, "lower case");
    expect("10002F0132-148006A", P::NotHex, u6, "minus sign (a negative number printed)");
    expect("60000000000000000A", P::UnknownCode, u6, "code 60");

    // 3.01's positive-grid bug: for +1 V, %04X printed the negative grid number as
    // eight digits (FFFFFF7E) and the 18-character cut-off left "FFFF" as the grid
    // word and "FF7E" as the heater word. The heater word gives it away.
    expect("10002F0132FFFFFF7E", P::Heater, u6, "3.01's +1 V command on a uTracer6");
    expect("10002F0132FFFF006A", P::None, u6, "uTracer6: FFFF on its own is a legal grid word");
    expect("10002F0132FFFF006A", P::Grid, u3, "uTracer3: FFFF grid refused");

    // Anode and screen against the limit
    expect("10013201320148006A", P::None, u6lim, "anode at the limit word");
    expect("10013301320148006A", P::Anode, u6lim, "anode one count over");
    expect("10013201330148006A", P::Screen, u6lim, "screen one count over");
    expect("1004000000000000000", P::Length, u6, "19 characters");
    expect("100400000000000000", P::Anode, u6, "anode word above 3FF");

    // Grid, uTracer3
    expect("10002F013203FF006A", P::None, u3, "uTracer3: 3FF");
    expect("10002F01320400006A", P::Grid, u3, "uTracer3 10-bit: 400 refused");
    expect("10002F0132C3FF006A", P::None, u3fw203, "uTracer3 fw 203+: C3FF");
    expect("10002F01323000006A", P::Grid, u3fw203, "uTracer3 fw 203+: 3000 refused");
    // uTracer6: every word grid_word.h can make is allowed
    char buf[19];
    for (float v = -150; v <= 150; v += 0.5f) {
        std::snprintf(buf, sizeof buf, "10002F0132%04X006A", utracer6GridWord(v, 100));
        expect(buf, P::None, u6, "uTracer6 grid word from utracer6GridWord()");
    }

    // Heater
    expect("10002F0132014803FF", P::None, u6, "heater 3FF");
    expect("10002F013201480400", P::Heater, u6, "heater 400 (Vh above the supply)");
    expect("4000000000000005C1", P::Heater, u6, "heater command 5C1 (18 V on a 15 V supply)");
    expect("10002F01320148E3FF", P::None, ext, "external heater E3FF");
    expect("10002F01320148E3FF", P::Heater, u6, "E3FF without the external heater option");

    // Settings
    expect("000000000080010000", P::None, u6, "averaging 1, gains 0");
    expect("000000000080030000", P::Settings, u6, "averaging 3");
    expect("000000000080400900", P::Settings, u6, "screen gain 9");
    expect("000000010080400808", P::NotZero, u6, "settings word 2 not zero");

    // Unused words
    expect("300000000000000001", P::NotZero, u6, "30 with a non-zero word");
    expect("500000000100000000", P::NotZero, u6, "50 with a non-zero word");
    expect("400001000000000056", P::NotZero, u6, "40 with a non-zero anode word");

    // Nick Barton's firmware commands are not Dekker commands
    if (isDekkerCommand("54") || isDekkerCommand("5556") || isDekkerCommand("51abc")) {
        std::printf("FAIL: a Nick Barton firmware command taken for a Dekker command\n");
        failures++;
    }
    if (!isDekkerCommand("300000000000000000")) {
        std::printf("FAIL: 30 not recognised\n");
        failures++;
    }

    if (failures == 0) std::printf("all passed\n");
    return failures ? 1 : 0;
}
