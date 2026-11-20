# Changes from uTmax 3.01

The GPL asks a modified program to say what was changed and when. This file does
that in plain words, newest first. The first commit in this repository is Nick
Barton's uTmax 3.01 exactly as he published it, so `git diff <first commit>` shows
every line that differs. Each entry gives the reason, and credits Dean Jenkins
where the fix comes from his fork.

## 2026-11-20

- **Fixed: crash on the first point of every sweep when the style sheet can't be
  opened.** 3.01 built its plot-line pens only while reading the style sheet named
  in `cal.txt`. If that file was missing (moved, renamed, a theme not copied to a
  new machine; a new install points at `style.qss` in the program folder), the
  pen list stayed empty and `PlotTabWidget::plotUpdate()` took
  `gpnum % penList->length()`: a division by zero, so uTmax died (SIGFPE) on the
  first plotted point. The same code could also read a pen past the end of the
  list when the curve number jumped by more than one, or with "colour by group".
  New `src/plot_pens.h`: the 50 default pens (the same colours as before) are
  made when the main window is created (`src/mainwindow.cpp`), the style sheet
  still recolours them, and every pen in `src/plotTabWidget.cpp` is fetched
  through `penAt()`, which extends the list as needed. Tests:
  `tests/test_plot_pens.py`.
- **Fixed: the negative supply reading.** The uTracer6 reports its −85 V rail,
  measured through R27 200 kΩ / R29 4.7 kΩ (Dekker, uTracer6 log, fig. 7.2 and
  section 9). 3.01 decoded it with the −15 V rail's divider (47 kΩ / 4.7 kΩ), so a
  healthy uTracer6 showed about −17.7 V in the debug window and the calibration
  wizard, and the wizard would have set a calibration factor near 4.8 to make up
  for it. For the uTracer3 3.01 wrote the divider as `(47 + 2)/2`, which C++
  works out as integer division (24, not 24.5), so the reading was about 2 % off
  (−39.1 V for −40 V). New `src/neg_rail.h` holds both dividers and the
  conversion; `Update_adc_scales()` and `GetReal()` in `src/mainwindow.cpp` use
  it. The test hook's report now includes the negative-rail and supply readings
  (`src/test_hook.cpp`). Tests: `tests/test_neg_rail.py`. The simulator's
  uTracer6 profile now reports −85 V through 200 kΩ / 4.7 kΩ.

## 2026-11-18

- **Fixed: calibration values were never initialised.** 3.01 gave its
  calibration values (`calData`) no defaults, and the main window is created on
  the stack, so any key missing from `cal.txt` took whatever was in memory. In
  test runs the anode factor came out as −89 254 and as 2.5 × 10³¹: uTmax sent
  the anode at 0 V and the grid at 0 V instead of −2 V, still reported the sweep
  as done, and wrote the garbage back to `cal.txt`. The `cal.txt` it created on a
  new install wrote obsolete keys (`Vg05`, `Vg1`, `Vg4`, `Vg40`) instead of `Vg0`
  and `Vgm`, so every new install ran with an uninitialised grid calibration
  until the calibration wizard was used. An unreadable value (`Va=abc`) became 0.
  Now: every field of `calData_t` (`src/mainwindow.h`) has a default (the values
  3.01's own new-install file and calibration reset use: factors 1, grid offset
  0, 6800 Ω, 300 V, 18 Ω, 50 V); the new-install `cal.txt` writes `Vg0=0` and
  `Vgm=1.0`; `ReadCalibration()` keeps the default (with a warning) for a value
  that is not a finite number, and skips a line without `=` (3.01 read past the
  end of it). Tests: `tests/test_calibration_defaults.py`.
- **Test hook: `uTmax --test-sweep <file>`** (for automated tests only; no
  change to how uTmax behaves without the option). New `src/test_hook.cpp`:
  waits until uTmax has opened the port and is idle, picks the tube, fills in the
  sweep fields from the file, presses Start, lets the normal state machine run
  the sweep, records every status-bar message and error dialog (closing the
  dialog so a test can't hang), and writes a JSON report with every measured
  point before quitting. `src/main.cpp` reads the option; `src/mainwindow.h`
  declares the four hook functions and their state; `src/uTmax.pro` builds the
  new file. Used by the new end-to-end tests in `tests/test_sweeps.py`.
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
