"""Known bugs in uTmax 3.01, recorded as tests before they are fixed.

Each test states what uTmax 3.01 does. When a bug is fixed, its test moves to a
test of the correct behaviour in the same commit, and the bug is listed below.

Open: none recorded as a test yet. The list of known problems is in
docs/PLAN.md (stage 4) and docs/DEVELOPER.md (section 11).

Fixed:
- 2026-11-18  Positive grid on a uTracer6 sent as FFFF (+100 V), heater word lost.
              Now tests/test_grid_word.py; 3.01 reproduced in
              tests/repro/positive_grid_word.cpp.
"""
