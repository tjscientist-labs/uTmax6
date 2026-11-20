/* reply_format.h — the uTracer's result string: its length and its status byte.
 *
 * Part of uTmax6, a fork of uTmax by Nick Barton. GNU GPL v3 or later.
 *
 * Commands 10 (measure) and 50 (read ADC) are answered, after the echo, with a
 * result string of hex characters. Its first byte is the status
 * (Dekker, uTracer3 log, "Communication protocol"):
 *
 *     10H  measurement OK
 *     11H  current limit (compliance) on the anode or screen
 *
 * With Ronald Dekker's positive-grid extension board fitted to a uTracer6, the
 * firmware sets the highest bit of the status (90H / 91H) and appends four hex
 * characters, the ADC reading of the grid current (AN12), so the string is 42
 * characters instead of 38. Dekker, uTracer6 construction log, section 14:
 *
 *     "Four characters are appended to the result string ... At the same time
 *      the highest bit in the status word is made 1 to signal the GUI that it
 *      can expect 42 characters instead of the usual 38."
 *     https://www.dos4ever.com/uTracerlog6/tubetester6.html
 *
 * The fields of the 38-character string keep their positions, so code that
 * reads them needs no change; only the length and the status test do.
 *
 * Why this is a separate file: uTmax 3.01 waited for exactly 38 characters and
 * compared the status with the text "11". With the board fitted the 42-character
 * reply was never accepted (uTmax timed out and reconnected, again and again),
 * and a current-limit status 91H would not have been recognised. Keeping the
 * rules here lets them be tested on their own (tests/cpp/test_reply_format.cpp).
 */
#ifndef REPLY_FORMAT_H
#define REPLY_FORMAT_H

#include <string>

constexpr int RESULT_LEN = 38;          // result string, standard uTracer
constexpr int RESULT_LEN_GRID_BOARD = 42; // with the positive-grid extension board

constexpr int STATUS_GRID_BOARD = 0x80;   // bit 7: extension board fitted
constexpr int STATUS_CURRENT_LIMIT = 0x11; // with bit 7 masked off

// One hex digit to its value, or -1 if it is not a hex digit.
inline int hexDigit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// The status byte from its two hex characters, or -1 if they are not hex.
inline int replyStatus(char hi, char lo)
{
    int h = hexDigit(hi), l = hexDigit(lo);
    if (h < 0 || l < 0) return -1;
    return h * 16 + l;
}

// True if the status says the positive-grid extension board is fitted.
inline bool statusHasGridBoard(int status)
{
    return status >= 0 && (status & STATUS_GRID_BOARD) != 0;
}

// True if the status reports a current limit (11H, or 91H with the board).
inline bool statusIsCurrentLimit(int status)
{
    return status >= 0 && (status & ~STATUS_GRID_BOARD) == STATUS_CURRENT_LIMIT;
}

// How many characters the result string has, given its status byte.
// An unreadable status gives the standard length; the reply is then checked
// (and rejected) by the code that reads it, as before.
inline int resultLength(int status)
{
    return statusHasGridBoard(status) ? RESULT_LEN_GRID_BOARD : RESULT_LEN;
}

// uTmax6: is a result string (after the echo) one uTmax can use? 3.01 read every
// field with toInt(), so a character that wasn't hex silently became 0 and the
// point was stored, and it used the two gain codes (characters 34-37) to index
// an 8-entry table without checking them. Checked here:
//   - 38 characters, or 42 when the status says the grid board is fitted
//   - every character a hex digit
//   - with Dekker's firmware (knownStatusOnly): status 10H or 11H, bit 7 aside
//   - both gain codes 0-7
enum class ReplyProblem { None, Length, NotHex, Status, Gain };

inline const char *replyProblemText(ReplyProblem p)
{
    switch (p) {
    case ReplyProblem::None:   return "OK";
    case ReplyProblem::Length: return "wrong length";
    case ReplyProblem::NotHex: return "a character that is not hex";
    case ReplyProblem::Status: return "unknown status byte";
    case ReplyProblem::Gain:   return "gain code out of range";
    }
    return "?";
}

inline ReplyProblem checkReply(const std::string &r, bool knownStatusOnly)
{
    for (char ch : r)
        if (hexDigit(ch) < 0) return ReplyProblem::NotHex;
    if (r.size() < 2) return ReplyProblem::Length;
    const int status = replyStatus(r[0], r[1]);
    if (int(r.size()) != resultLength(status)) return ReplyProblem::Length;
    if (knownStatusOnly) {
        const int s = status & ~STATUS_GRID_BOARD;
        if (s != 0x10 && s != STATUS_CURRENT_LIMIT) return ReplyProblem::Status;
    }
    const int ga = hexDigit(r[34]) * 16 + hexDigit(r[35]);
    const int gs = hexDigit(r[36]) * 16 + hexDigit(r[37]);
    if (ga > 7 || gs > 7) return ReplyProblem::Gain;
    return ReplyProblem::None;
}

#endif // REPLY_FORMAT_H
