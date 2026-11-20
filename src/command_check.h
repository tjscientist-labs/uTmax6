/* command_check.h — the last check on a command before it goes to the uTracer.
 *
 * Part of uTmax6, a fork of uTmax by Nick Barton. GNU GPL v3 or later.
 *
 * Ronald Dekker's protocol has no checksum (uTracer3 log, "Communication
 * protocol"; uTracer6 log, section 8). Every command is 18 upper-case hex
 * characters: a 2-digit code and four 16-bit words. The uTracer acts on what it
 * receives, so a wrong command is a wrong voltage on the tube. uTmax 3.01 sent
 * whatever it had built; that is how a positive grid on a uTracer6 went out as
 * +100 V (grid_word.h).
 *
 * checkCommand() looks at the finished text, independently of how it was built:
 *
 *   all     18 characters, 0-9 and A-F only, code 00 10 20 30 40 or 50
 *   00      words 1 and 2 zero; averaging 1 2 4 8 16 32 or 40H (auto);
 *           screen and anode gain 0-8 (8 = auto); any compliance byte
 *   10, 20  anode and screen no higher than the words for the voltage limit
 *           (voltage_limit.h, worked out by the caller with the same
 *           conversion that built the command); grid: uTracer3 10-bit, or
 *           12-bit split as uTmax packs it for firmware 203+; uTracer6 any
 *           16-bit word (sign and magnitude); heater 0-3FFH, or E000H plus
 *           0-3FFH with an external heater supply
 *   30, 50  all four words zero (uTmax sends zeros; the firmware ignores them)
 *   40      words 1-3 zero; heater as above
 *
 * Commands that don't start with one of those codes are Nick Barton's own
 * firmware commands (51, 52, 54, 55, 57, 59, and the boot-loader's raw bytes);
 * isDekkerCommand() says no and they are left alone.
 *
 * No Qt here, so it can be tested on its own: tests/cpp/test_command_check.cpp.
 */
#ifndef COMMAND_CHECK_H
#define COMMAND_CHECK_H

#include <string>

struct CommandLimits {
    unsigned maxAnodeWord = 0x3FF;   // the anode word for the voltage limit
    unsigned maxScreenWord = 0x3FF;  // the screen word for the voltage limit
    bool uTracer6 = false;
    bool grid12bit = false;          // uTracer3 with firmware 203 or later
    bool extHeater = false;          // Options > External Heater
};

enum class CmdProblem { None, Length, NotHex, UnknownCode, Settings, Anode, Screen,
                        Grid, Heater, NotZero };

inline const char *problemText(CmdProblem p)
{
    switch (p) {
    case CmdProblem::None:        return "OK";
    case CmdProblem::Length:      return "not 18 characters";
    case CmdProblem::NotHex:      return "not upper-case hex";
    case CmdProblem::UnknownCode: return "unknown command code";
    case CmdProblem::Settings:    return "settings byte out of range";
    case CmdProblem::Anode:       return "anode voltage above the limit";
    case CmdProblem::Screen:      return "screen voltage above the limit";
    case CmdProblem::Grid:        return "grid value out of range";
    case CmdProblem::Heater:      return "heater value out of range";
    case CmdProblem::NotZero:     return "unused field not zero";
    }
    return "?";
}

// True if the text starts with one of Dekker's six command codes.
inline bool isDekkerCommand(const std::string &c)
{
    if (c.size() < 2) return false;
    const std::string code = c.substr(0, 2);
    return code == "00" || code == "10" || code == "20" || code == "30"
        || code == "40" || code == "50";
}

namespace command_check_detail {
inline int hexValue(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}
inline unsigned word(const std::string &c, int n)   // n = 1..4
{
    unsigned v = 0;
    for (int i = 0; i < 4; i++) v = v * 16 + unsigned(hexValue(c[2 + 4 * (n - 1) + i]));
    return v;
}
inline unsigned byteAt(const std::string &c, int pos)   // pos = character index
{
    return unsigned(hexValue(c[pos]) * 16 + hexValue(c[pos + 1]));
}
inline bool heaterOK(unsigned h, bool ext)
{
    if (h <= 0x3FF) return true;
    return ext && (h & 0xFC00) == 0xE000;
}
}

inline CmdProblem checkCommand(const std::string &c, const CommandLimits &lim)
{
    using namespace command_check_detail;
    if (c.size() != 18) return CmdProblem::Length;
    for (char ch : c)
        if (hexValue(ch) < 0) return CmdProblem::NotHex;
    if (!isDekkerCommand(c)) return CmdProblem::UnknownCode;
    const std::string code = c.substr(0, 2);

    if (code == "00") {
        if (word(c, 1) != 0 || word(c, 2) != 0) return CmdProblem::NotZero;
        const unsigned avg = byteAt(c, 12), gs = byteAt(c, 14), ga = byteAt(c, 16);
        const bool avgOK = avg == 1 || avg == 2 || avg == 4 || avg == 8 || avg == 16
                        || avg == 32 || avg == 0x40;
        if (!avgOK || gs > 8 || ga > 8) return CmdProblem::Settings;
        return CmdProblem::None;
    }
    if (code == "10" || code == "20") {
        if (word(c, 1) > lim.maxAnodeWord || word(c, 1) > 0x3FF) return CmdProblem::Anode;
        if (word(c, 2) > lim.maxScreenWord || word(c, 2) > 0x3FF) return CmdProblem::Screen;
        const unsigned g = word(c, 3);
        if (!lim.uTracer6) {
            const unsigned mask = lim.grid12bit ? 0xC3FF : 0x03FF;
            if (g & ~mask) return CmdProblem::Grid;
        }
        if (!heaterOK(word(c, 4), lim.extHeater)) return CmdProblem::Heater;
        return CmdProblem::None;
    }
    if (code == "40") {
        if (word(c, 1) || word(c, 2) || word(c, 3)) return CmdProblem::NotZero;
        if (!heaterOK(word(c, 4), lim.extHeater)) return CmdProblem::Heater;
        return CmdProblem::None;
    }
    // 30 and 50
    if (word(c, 1) || word(c, 2) || word(c, 3) || word(c, 4)) return CmdProblem::NotZero;
    return CmdProblem::None;
}

#endif // COMMAND_CHECK_H
