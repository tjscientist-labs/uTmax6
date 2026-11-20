# Changes from uTmax 3.01

The GPL asks a modified program to say what was changed and when. This file does
that in plain words, newest first. The first commit in this repository is Nick
Barton's uTmax 3.01 exactly as he published it, so `git diff <first commit>` shows
every line that differs. Each entry gives the reason, and credits Dean Jenkins
where the fix comes from his fork.

## 2026-11-20

- **New: a positive grid only with the positive-grid board.** 3.01 allowed a
  positive grid on any uTracer6. `RxData()` in `src/mainwindow.cpp` now records
  status bit 7 of each usable reply (`gridBoardFitted`, `src/mainwindow.h`);
  `SetUpSweepParams()` refuses a positive grid without it, and the command check
  refuses a uTracer6 grid word with bit 15 set without it (`positiveGridOK` in
  `src/command_check.h`). Tests: `tests/test_positive_grid.py`; cases in
  `tests/cpp/test_command_check.cpp`.
- **New: replies are checked before they are used.** 3.01 read each field of a
  result string with `toInt()`, so a character that wasn't hex became 0 and the
  point was stored, and it indexed an 8-entry gain table with the reply's gain
  codes unchecked. New `checkReply()` in `src/reply_format.h`: length as the
  status says, all hex, status `10`/`11` (bit 7 aside) with Dekker's firmware,
  gain codes 0-7. `RxData()` in `src/mainwindow.cpp` checks every reply to `10`
  and `50`; one that fails stops the sweep with End Measurement (`30`) and a
  message. The three stop-and-discharge paths (refused command, wrong echo,
  unusable reply) now share `abortAndDischarge()` (`src/mainwindow.h`). Tests:
  `tests/test_bad_reply.py`; cases added to `tests/cpp/test_reply_format.cpp`.
- **New: a wrong echo stops the sweep and discharges.** In `RxData()`
  (`src/mainwindow.cpp`) an echo that doesn't match the command (`RXINVALID`)
  made 3.01 go idle with the reservoirs as they were. uTmax6 sends End
  Measurement (`30`) at once and shows the reason; if the `30` is echoed wrongly
  too, it goes idle as before. The simulator gained `--bad-echo-on N`
  (`tools/utracer_sim/utracer_sim.py`, its README; a test in
  `tests/test_simulator.py`). Test: `tests/test_bad_echo.py`.
- **New: every command is checked before it is sent.** Dekker's protocol has no
  checksum and 3.01 sent whatever it had built. New `src/command_check.h`:
  `checkCommand()` checks each of Dekker's commands (`00`-`50`) for form and for
  every field's range on the uTracer model in use, with the anode and screen
  words against the voltage limit. `sendSer()` in `src/mainwindow.cpp` calls the
  new `commandAllowed()`; a command that fails is not sent, End Measurement
  (`30`) is sent instead, and the reason stays in the status bar
  (`afterDischargeMessage`, `Discharge_Done()`; `src/mainwindow.h`). Nick
  Barton's own firmware commands are not checked. This also stops a heater
  setting above the supply voltage, which 3.01 sent as a heater word beyond
  10 bits (`GetVf()` has no upper clamp). The simulator gained `--supply-volts`
  (`tools/utracer_sim/utracer_sim.py`, its README) to test that. Tests:
  `tests/test_command_check.py`, `tests/cpp/test_command_check.cpp`, and a new
  case in `tests/test_simulator.py`.
- **Small fixes** (no change to what uTmax sends or measures):
  - The `data.csv` header written by `SaveTubeDataFile()` named the gm2
    tolerance column `gm1Del`, so `gm1Del` appeared twice; now `gm2Del`. From
    Dean Jenkins' fork. Test: `HeaderWrittenCorrectly` in
    `tests/test_tube_library.py`.
  - `GetVg()` read `Vg` before setting it (`if (Vg<0) Vg=0;`, overwritten on
    the next line); removed.
  - `readData()` called `d.remove(i,0)`, which removes nothing, for CR, LF and
    spaces; they were and are dropped by not being appended. Rewritten to say so.
  - The results of four `QFile::open()` calls (cal.txt create and read, the
    tube library, both logs) are now checked, with a warning on failure.
  - `size` in `dr_optimize.cpp` is initialised (printed in the debug output
    even when the first step fails).
  - The deliberate fall-through in `Pgm_Flash()` is marked `[[fallthrough]]`.
  - The idle heartbeat's status messages "Send Ping" / "Ping OK" are now "Idle:
    discharge sent" / "Idle: uTracer answered": the heartbeat sends End
    Measurement (`30`), and Dekker's "ping" is a different command (`50`).
    Comment updated in `tests/test_utmax_smoke.py`.
- **Fixed: crash at start-up when the tube library has no usable tube.**
  `ReadDataFile()` in `src/mainwindow.cpp` took the first tube
  (`tubeDataVec.at(0)`) without checking there was one, so an empty, header-only
  or damaged `data.csv` made uTmax abort (`std::out_of_range`) before its window
  opened. `ReadDataFile()` now returns on an empty library, and the constructor
  renames the file `data.csv.unusable-<date-time>`, installs the built-in library
  (`installDefaultFile()`, `src/app_paths.h`) and shows a message once the window
  is up. Tests: `tests/test_tube_library.py`; `tests/utmax_run.py` can write a
  test's own `data.csv`.
- **Fixed: first-start settings, and themes that work from any folder.** The
  `cal.txt` 3.01 created named the serial port `COM1` (a Windows name) and the
  style as `<program folder>/style.qss`, which doesn't exist; and the themes
  find their images as `url(./Themes/img/...)`, relative to the folder uTmax was
  started from. New `src/new_install.h`: `ReadCalibration()` in
  `src/mainwindow.cpp` writes the first USB serial port seen
  (`defaultSerialPort()`; else `/dev/ttyUSB0` on Linux, `COM1` on Windows) and
  `~/uTmax_files/Themes/Utracer Dark.qss`; `src/main.cpp` installs the themes
  and their images, now built into the program (`src/utmax6.qrc`), where
  missing; `SetStyleSheet()` makes a theme's `./` paths absolute
  (`resolveThemeUrls()`). `VaMax` stays 300 V on a new install, on purpose. The
  test hook can start without pressing Start (`Start=0`) and reports the
  window's theme (`src/test_hook.cpp`). Tests: `tests/test_new_install.py`.
- **Fixed: `~/uTmax_files` is created, and the style sheet and logs live there.**
  3.01 never created the folder: on a new machine it opened a file dialog asking
  for `data.csv`, and `cal.txt` was never written, so every setting was lost at
  exit. It read `Style.qss` from the folder it was started in and wrote its logs
  to the home and current folders. New `src/app_paths.h`: `src/main.cpp` creates
  the folder, copies in the tube library built into the program (new
  `src/utmax6.qrc` with `src/data.csv`, added to `RESOURCES` in `src/uTmax.pro`)
  if there is none, and loads `Style.qss` from the folder first, then the
  program's folder, then the current one. `src/mainwindow.cpp` uses the same
  folder for its paths and both log files. The test hook reports the style sheet
  in use (`src/test_hook.cpp`). Tests: `tests/test_app_paths.py`;
  `tests/utmax_run.py` gained `fresh`, `start_in` and `app_style`. After Dean
  Jenkins' fork, which checked for the folder.
- **Fixed: `cal.txt` is written only when it changes, and safely.** 3.01 rewrote
  `cal.txt` whenever the serial port opened (every start and every successful
  reconnect), on every exit, and after plot and pen updates, changed or not:
  needless writes to a Raspberry Pi's SD card, and a power cut during one could
  leave a half-written file. `SaveCalFile()` in `src/mainwindow.cpp` now builds
  the text and calls `writeFileIfChanged()` (new `src/cal_file.h`), which does
  nothing if the file already holds that text and otherwise writes through
  `QSaveFile` (a temporary file renamed into place). The file is now written
  with `\n` line endings on every system. After Dean Jenkins' fork, which
  stopped the start-up rewrite.
- **Fixed: the tube last used is remembered.** 3.01 read `Index=` from `cal.txt`
  before the tube list was loaded, so it was lost, every start came up on the
  first tube, and `Index=0` was written back. The value is now kept
  (`savedTubeIndex`, `src/mainwindow.h`) and applied at the end of start-up.
  Tests for both: `tests/test_cal_file.py`; `tests/utmax_run.py` now reports the
  file's inode and modification time before and after a run, fills in `{home}`
  in a test's `cal.txt`, and can keep uTmax's debug output (`UTMAX_STDERR=`).
- **Fixed: the reservoirs are discharged as soon as a sweep ends.** Every way out
  of a sweep (the last point, Stop, a current-limit abort, the voltage backstop)
  goes through `End()`, which sends a last measurement at 4 V. 3.01 then went
  straight to idle, and End Measurement (`30`: discharge the anode and screen
  reservoirs, grid to 0 V) went out only with the 20-second idle heartbeat, so
  the reservoirs could stay charged for up to 20 s. `HeatOff_f()` in
  `src/mainwindow.cpp` now sends `30` and goes idle when it is echoed (new state
  `discharge` and `Discharge_Done()` in `src/mainwindow.h`). The heater is not
  touched; the heartbeat has always sent `30` with the heater held. After Dean
  Jenkins' fork, which sends it on Stop. The test hook can now press Stop
  part-way (`StopAfter=`, `src/test_hook.cpp`). Tests: `tests/test_discharge.py`.
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
