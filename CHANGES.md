# Changes from uTmax 3.01

The GPL asks a modified program to say what was changed and when. This file does
that in plain words, newest first. The first commit in this repository is Nick
Barton's uTmax 3.01 exactly as he published it, so `git diff <first commit>` shows
every line that differs. Each entry gives the reason, and credits Dean Jenkins
where the fix comes from his fork.

## 2026-11-18

- **Fixed: delayed sweeps stopped above 425 V on a uTracer6.** In 3.01 the
  hold path (sweeps with a Delay set) refused any anode or screen voltage above
  a fixed 425 V, the uTracer3's ceiling, and stopped the sweep with "Internal
  Error: Va or Vs excessive". Sweeps without a delay had no such backstop at
  all. Every measurement (both paths, three places in `src/mainwindow.cpp`:
  `Sweep_Set()` twice and `Hold()`) now goes through `hvWithinLimit()` in the
  new `src/voltage_limit.h`: the user's `VaMax`, capped at 425 V on a uTracer3
  (3.01's figure, so the uTracer3 behaves as before) and 1000 V on a uTracer6
  (Dekker's rating), with 1 V of slack for rounding in the sweep steps. A NaN
  voltage is refused (3.01 let it through). The sweep set-up still checks and
  clamps against `VaMax` as before; this is the backstop behind it. Tests:
  `tests/test_voltage_limit.py`. A full sweep is not yet driven end to end.
- **Fixed: 42-character replies from the positive-grid extension board.** With
  Dekker's extension board fitted, a uTracer6 sets bit 7 of the status byte and
  appends a four-character grid-current word, so its replies are 42 characters
  (Dekker, uTracer6 log, section 14). uTmax 3.01 waited for exactly 38, so it
  never accepted a reply: it timed out, closed the port, reopened it and tried
  again (three times in 26 seconds against the simulator), and never reached
  idle. `RxPkt()` in `src/mainwindow.cpp` now reads the status byte first and
  waits for 38 or 42 characters accordingly, as Dekker's own GUI does. The two
  current-limit checks (`Sweep_adc()` and `saveADCInfo()`) compared the status
  with the text "11", which would have missed `91` (a current limit with the
  board fitted); they now mask bit 7. The rules are in the new
  `src/reply_format.h`. The grid-current word is received but not yet shown;
  that comes with the extension-board option. The uTracer3 and a uTracer6
  without the board behave exactly as before. Tests: `tests/test_reply_format.py`
  and `UTmaxWithPositiveGridBoard` in `tests/test_utmax_smoke.py`.
- **Fixed: positive grid voltage on a uTracer6.** uTmax 3.01 sent a positive
  grid voltage as a negative number; it was printed as eight hex digits and the
  command was cut off, so +1 V reached the uTracer6 as grid word `FFFF` (+100 V)
  and the heater setting was lost. The uTracer6 grid word is now built by
  `utracer6GridWord()` in the new `src/grid_word.h`, exactly as Dekker specifies:
  magnitude in bits 0-14 (100 V full scale), bit 15 set for a positive grid,
  always four hex digits. +1 V is now `8148`. `GetVg()` in `src/mainwindow.cpp`
  calls it; the uTracer3 path is unchanged. Tests: `tests/test_grid_word.py`.
- **Finding: positive grid on a uTracer6** (fixed the same day, above).
- **Simulator:** uTracer6 profile checked against Dekker's uTracer6 page; grid
  words decoded as Dekker's sign and magnitude; `--grid-board` option for the
  positive-grid extension (status bit 7, 42-character replies).
- **Software uTracer and first tests** (no change to the program). Added
  `tools/utracer_sim/`, a simulator of the uTracer on a virtual serial port, and
  `tests/`. The first end-to-end test records what uTmax 3.01 does on its own:
  it reads the ADCs when the port opens (`50`), and sends End Measurement (`30`)
  every 20 seconds while idle, which it labels "Send Ping".
- **Finding (not yet changed):** uTmax's default uTracer3 anode divider (470 kΩ
  with 6.8 kΩ) differs from Dekker's published 482 kΩ with 6.81 kΩ, so it reads
  about 2.4 % low before calibration. Calibration absorbs it.
- **Windows build** in CI: install MSYS2's `mingw-w64-ucrt-x86_64-make`, which
  provides `mingw32-make`.
- **Build.** `src/uTmax.pro` linked the GNU Scientific Library only on Windows,
  from paths on Nick Barton's own PC. It now links `-lgsl -lgslcblas` from the
  system on every platform. *Why:* so the same project file builds on Linux, the
  Raspberry Pi and Windows (MSYS2) without editing. No program code changed.
- **Repository.** Added a README, credits, this file, build instructions, the plan
  and progress list, a `.gitignore`, and automatic builds (GitHub Actions) for
  Linux x64, Linux arm64 and Windows.
- **Import.** Nick Barton's 3.01 source, unchanged. Build output, Windows
  binaries, generated files and machine-specific Qt Creator settings were left
  out; the themes moved from `release/Themes` to `src/Themes`. The import commit
  lists every exclusion.
