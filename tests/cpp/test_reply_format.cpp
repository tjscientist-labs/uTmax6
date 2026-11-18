// Tests for src/reply_format.h: result-string length and status byte
// (Dekker, uTracer3 protocol; uTracer6 log section 14 for the extension board).
// Built and run by tests/test_reply_format.py, or by hand:
//   g++ -std=c++17 -Wall -o /tmp/trf tests/cpp/test_reply_format.cpp && /tmp/trf
// Exit status 0 = all passed.
#include "../../src/reply_format.h"
#include <cstdio>

static int failures = 0;

static void check(bool ok, const char *what) {
    if (!ok) { std::printf("FAIL: %s\n", what); failures++; }
}

int main() {
    // Status parsing
    check(replyStatus('1', '0') == 0x10, "10 -> 0x10");
    check(replyStatus('9', '1') == 0x91, "91 -> 0x91");
    check(replyStatus('a', 'f') == 0xAF, "lower-case hex accepted");
    check(replyStatus('Z', 'Z') == -1, "ZZ -> -1");
    check(replyStatus('1', ' ') == -1, "space -> -1");

    // Length: 38 normally, 42 when bit 7 says the board is fitted
    check(resultLength(0x10) == 38, "10H -> 38");
    check(resultLength(0x11) == 38, "11H -> 38");
    check(resultLength(0x90) == 42, "90H -> 42");
    check(resultLength(0x91) == 42, "91H -> 42");
    check(resultLength(-1) == 38, "unreadable status -> 38");

    // Board detection
    check(!statusHasGridBoard(0x10), "10H: no board");
    check(statusHasGridBoard(0x90), "90H: board");
    check(!statusHasGridBoard(-1), "unreadable: no board");

    // Current limit: 11H and 91H, nothing else
    check(!statusIsCurrentLimit(0x10), "10H is not a current limit");
    check(statusIsCurrentLimit(0x11), "11H is a current limit");
    check(!statusIsCurrentLimit(0x90), "90H is not a current limit");
    check(statusIsCurrentLimit(0x91), "91H is a current limit (3.01 missed this)");
    check(!statusIsCurrentLimit(-1), "unreadable is not a current limit");

    if (failures == 0) std::printf("all reply-format checks passed\n");
    return failures == 0 ? 0 : 1;
}
