# Progress

What has been done on uTmax6, and what comes next. Newest first in each list. The
reasons behind the order are in [`PLAN.md`](PLAN.md); every code change is
described in [`CHANGES.md`](../CHANGES.md).

**Last updated:** 2026-11-20

## The software uTracer at work

The simulator (`tools/utracer_sim/`) is built, tested and in daily use. It
pretends to be a uTracer3 or uTracer6 on a virtual serial port, answers Ronald
Dekker's protocol character for character, models a resistor, diode or 12AX7
load, can misbehave on purpose (no reply, garbled reply, current-limit trip, the
positive-grid board's longer replies), and logs every command it receives,
decoded into volts.

**How it is used.** Every change to uTmax6 goes through it, three ways:

1. **Decoding single commands.** Small C++ checks build the exact command uTmax
   would send, and the simulator decodes it as the uTracer would, so an encoding
   is checked against the instrument, not against itself.
2. **The real program, idle.** uTmax runs with no window against the simulator
   and the test reads what it sent on its own: the ADC read at start-up and the
   20-second heartbeat. Run with and without the positive-grid board.
3. **The real program, whole sweeps.** A test hook (`uTmax --test-sweep`) lets a
   test pick a tube, set a sweep and press Start. The test then checks both
   sides: what the simulator received (the voltages actually asked for) and what
   uTmax reported (the currents it measured). Into a 100 kΩ load from 30 to 300 V,
   uTmax reports the current to within 0.2 %.

**The rule for every fix:** first make the bug happen in a test, then fix it, then
put the old code back in a scratch copy and check that the test fails. All 107
tests run on every push, on Linux x64 and arm64; Windows builds too.

**What it has found so far:**

| Found | How | Status |
|---|---|---|
| A positive grid voltage on a uTracer6 was sent as **+100 V** (+1 V went out as `FFFF`), and the heater setting was lost | Simulator decoding uTmax's command | **Fixed** |
| With Dekker's positive-grid board, uTmax **never accepted a reply** (42 characters, not 38): it timed out and reconnected over and over; a current-limit status `91` would have been missed | Simulator with the board fitted, real program | **Fixed** |
| Sweeps with a hold delay **stopped above 425 V** on a uTracer6; sweeps without a delay had no last-moment check at all | Code comparison with Dean Jenkins' fork; confirmed with a whole sweep | **Fixed**; a 600 V delayed sweep now runs end to end |
| **Calibration values never initialised**: a key missing from `cal.txt` took whatever was in memory (seen: an anode factor of −89 254, and 2.5 × 10³¹), uTmax sent 0 V and still called the sweep done; the `cal.txt` made on a new install had no grid calibration at all | First whole sweep through the test hook | **Fixed** |
| The **negative supply reading** on a uTracer6 showed about −17.7 V instead of −85 V (uTmax used the divider of the other negative rail); on a uTracer3 it was about 2 % off (integer division) | Dekker's answer to which rail the uTracer6 reports, checked against his schematic; then the simulator and a whole sweep | **Fixed** |
| **Crash on the first point of every sweep** when the style sheet can't be found (a division by zero in the plot code) | First whole sweep through the test hook | **Fixed** |
| After a sweep finished or was stopped, the **reservoirs could stay charged for up to 20 s**: the discharge command went out only with the idle heartbeat | Comparing with Dean Jenkins' fork; confirmed from the simulator's command log | **Fixed** |
| `cal.txt` was **rewritten at every start, reconnect and exit**, changed or not; and the **tube last used was never remembered** (it was restored before the tube list existed) | Dean Jenkins' fork; the second found by the test for the first | **Fixed** |
| On a **new machine** uTmax never created its `~/uTmax_files` folder: it asked for the tube library in a file dialog and never saved its settings; its style sheet and logs used whatever folder it was started from | Dean Jenkins' fork; confirmed with a fresh home folder (3.01 sits in the dialog) | **Fixed** |
| A new install's settings named the serial port **`COM1` even on Linux** and a **style sheet that doesn't exist**; the themes' images only showed when uTmax was started from one particular folder | Reading the code, then a fresh home folder | **Fixed** |
| A tube library with **no usable tube** (empty, header only, damaged) **crashed uTmax at start-up**, every time, until the file was fixed by hand | Checking an unchecked file read in the small-fixes list | **Fixed** |
| A **heater setting above the supply voltage** went out as a heater word beyond its 10-bit range (an 18 V heater on a weak 15 V supply: `5C1`) | Writing the command checker; the simulator's new `--supply-volts` | **Stopped** by the new command check |
| uTmax's default uTracer3 anode divider reads **2.4 % low** against Dekker's published values | Building the simulator's uTracer3 profile | Recorded; calibration absorbs it |

## Done

- **2026-11-20** — **Every command is checked before it is sent.** Dekker's
  protocol has no checksum, and uTmax used to send whatever it had built. Now
  each command is checked just before it goes out: its form, and every value
  against what the uTracer model accepts, with the anode and screen against the
  voltage limit. A command that fails is not sent; the reservoirs are
  discharged instead and the reason is shown. The check would have caught
  3.01's +100 V grid bug, and it found one more problem: a heater set above the
  supply voltage sends a value the uTracer can't take.
- **2026-11-20** — **Small fixes.** The tube library's header line no longer
  names one column twice (from Dean Jenkins' fork). The idle heartbeat now says
  "Idle: discharge sent" instead of "Send Ping". Five pieces of code that read
  unset values, did nothing, or ignored errors were tidied, with no change to
  what uTmax does. Checking those ignored errors found the start-up crash above.
- **2026-11-20** — **Fixed: a crash at start-up with an empty or damaged tube
  library.** uTmax picked the first tube without checking there was one, so it
  would not start at all. Now it keeps the bad file under another name,
  installs the built-in library, and says so.
- **2026-11-20** — **Fixed: first-start settings that work on Linux.** A new
  install now picks the first USB serial adapter it can see (3.01 wrote the
  Windows name `COM1`), and comes with its themes installed and the "Utracer
  Dark" theme selected (3.01 named a style file that didn't exist). Themes now
  find their images whichever folder uTmax is started from. The maximum
  voltage stays at 300 V on a new install, on purpose.
- **2026-11-20** — **Fixed: uTmax sets itself up on a new machine.** It now
  creates its `~/uTmax_files` folder if it is missing and puts its tube library
  there, so a fresh install needs no copying by hand. Before, it opened a file
  dialog asking for the tube library and never saved its settings. The style
  sheet and the logs now live in that folder too, so it no longer matters which
  folder uTmax is started from.
- **2026-11-20** — **Fixed: `cal.txt` is written only when something changes.**
  uTmax rewrote its settings file every time it started, reconnected or closed,
  even when nothing had changed: needless writes to a Raspberry Pi's SD card,
  and a power cut at the wrong moment could leave the calibration half written.
  Now it writes only when the text differs, and always through a temporary
  file swapped into place, so the file is never half written. Making the test
  pass turned up a second bug: uTmax never remembered the tube last used. Now
  it starts on that tube.
- **2026-11-20** — **Fixed: discharge straight after a sweep.** When a sweep
  finished or was stopped, uTmax left the anode and screen capacitors
  charged until its next 20-second heartbeat sent the discharge command. Now it
  sends it as soon as the sweep ends, whatever the reason: the last point,
  Stop, a current-limit trip or the voltage check. The simulator's log shows
  the discharge arriving well under a second after the last measurement.
- **2026-11-20** — **Fixed: crash without a style sheet.** If the style sheet
  named in `cal.txt` couldn't be opened, uTmax had no colours for its plot lines
  and crashed on the first point of every sweep. Now the default colours always
  exist, and the style sheet only changes them. A sweep with a missing style
  sheet now runs to the end.
- **2026-11-20** — **Fixed: the negative supply reading.** Ronald Dekker
  confirmed that the uTracer6 reports its −85 V rail; his schematic shows it is
  measured through 200 kΩ / 4.7 kΩ. uTmax used the divider of the −15 V rail and
  showed about −17.7 V. Now it shows −85 V, and the uTracer3 reading, about 2 %
  off from integer division, is exact too. The simulator's uTracer6 now reports
  −85 V.
- **2026-11-18** — **Fixed: calibration values never initialised.** Every
  calibration value now starts from a defined default; a new install's `cal.txt`
  includes the grid calibration; a value that can't be read keeps its default
  instead of becoming 0. Before the fix, a test with a partial `cal.txt` sent the
  anode at 0 V instead of 30-300 V and the grid at 0 V instead of −2 V, and still
  reported the sweep as done. Now the same test sweeps correctly.
- **2026-11-18** — **Test hook: whole sweeps in the tests.** `uTmax --test-sweep`
  runs one sweep from a settings file and writes a report. Four end-to-end sweep
  tests: a uTracer6 sweep into 100 kΩ (currents within 0.2 %), a delayed sweep to
  600 V, the same sweep with the positive-grid board, and a sweep above the
  maximum refused before anything is sent. Its first run found two more bugs
  (uninitialised calibration; a crash without a style sheet).

- **2026-11-18** — **Fixed: delayed sweeps stopped at 425 V on a uTracer6.**
  Sweeps with a delay refused anything above 425 V, the uTracer3's ceiling. Every
  measurement is now checked against the maximum set in the calibration, capped
  at 425 V on a uTracer3 (as before) and 1000 V on a uTracer6. Sweeps without a
  delay, which had no such check, now have the same one.
- **2026-11-18** — **Fixed: uTmax works with the positive-grid extension board.**
  With Dekker's board fitted, the uTracer6 answers with 42 characters instead of
  38. uTmax 3.01 waited for 38, so it never accepted an answer and kept
  reconnecting. It now reads the status byte first, as Dekker's own program does,
  and also recognises a current limit with the board fitted (`91`). Tested end to
  end against the simulator with and without the board. The grid current is
  received but not shown yet.
- **2026-11-18** — **Fixed: positive grid on a uTracer6.** A positive grid
  voltage now goes out as Dekker specifies (+1 V is `8148`, not `FFFF` = +100 V),
  and the heater setting is no longer lost. Tested against the simulator's
  uTracer6 model from −150 V to +150 V.
- **2026-11-18** — Decided: a plain, easy-to-build case alongside the steampunk trunk, with room for an optional Raspberry Pi and display.
- **2026-11-18** — **Developer manual** (`docs/DEVELOPER.md`): structure, state machine, a sweep step by step, protocol, conversions, files, simulator, tests, how to make a change.
- **2026-11-18** — **Found: a positive grid on a uTracer6 sends +100 V.** uTmax
  3.01 formats a positive grid voltage as a negative number; the command is cut
  short and the uTracer6 would apply +100 V instead of, say, +1 V. Reproduced
  and recorded as a test; fix next.
- **2026-11-18** — Simulator's uTracer6 profile checked against Dekker's uTracer6
  page; positive-grid extension board added (42-character replies).
- **2026-11-18** — Automatic builds pass on Windows as well; Linux runs the tests
  against the freshly built program on x64 and arm64.
- **2026-11-18** — Decided: keep supporting the uTracer3 and uTracer3+.
- **2026-11-18** — **Software uTracer** (`tools/utracer_sim/`): echoes and answers
  Dekker's protocol, models resistor, diode and 12AX7 loads, injects faults, logs
  every command. Its formulas are checked against Dekker's published figures.
- **2026-11-18** — **First end-to-end test**: the real uTmax, run without a window,
  talks to the simulator; its start-up and idle behaviour is recorded.
- **2026-11-18** — Automatic builds pass on Linux x64 and Linux arm64.
- **2026-11-18** — Repository created with Nick Barton's uTmax 3.01 imported
  byte-for-byte as the first commit.
- **2026-11-18** — Project file links GSL from the system on every platform;
  automatic builds on Linux x64, Linux arm64 (Raspberry Pi class) and Windows.
- **2026-11-18** — README, credits (Nick Barton, Dean Jenkins, Ronald Dekker,
  Norman Koren), change log, build instructions, plan.
- **2026-11-18** — Commit rules: every commit message names every file it
  changes and says why; a hook in `tools/hooks/` checks this.
- **2026-11-18** — Compared Dean Jenkins' uTmax 1.3a fork with 3.01, fix by fix;
  found the 425 V limit on delayed sweeps.
- **2026-11-17** — uTmax 3.01 confirmed to build unchanged on Linux with Qt 6.

## In progress

- Design of a 1000 V Matrix relay board to replace Nick Barton's.
- Dummy-load designs for the first hardware checks.

## Planned

### Software
- [x] Simulator: Dekker's protocol, a tube model, fault injection, command log
- [x] Simulator: uTracer6 constants checked against Dekker's page
- [ ] Simulator: Windows support; reservoir droop
- [x] **Tests that drive a full sweep (test hook in uTmax)**
- [ ] Tests that record 3.01's present behaviour
- [x] **Fix the positive-grid command on the uTracer6**
- [x] **Handle the positive-grid board's 42-character replies**
- [ ] Show the grid current from the extension board
- [ ] Allow a positive grid only when the extension board is fitted
- [x] **Check every command before sending**
- [ ] Treat a bad echo as an error and discharge
- [ ] Check replies (hex, status byte)
- [x] **Fix the 425 V limit on delayed sweeps**
- [x] **Initialise every calibration value; write `Vg0`/`Vgm` in a new `cal.txt`**
- [x] **Don't crash when the style sheet is missing**
- [x] **Send End Measurement (discharge) on Stop and at the end of each sweep**
- [x] **Stop rewriting `cal.txt` at every start**
- [x] **Create `~/uTmax_files` if missing; load styles and write logs there**
- [x] **Sensible new-install defaults for Linux** (`VaMax` stays 300 V until the uTracer6 is checked)
- [x] **Smaller fixes** (see `PLAN.md`, stage 4)
- [ ] Check the uTracer6 conversion constants against Dekker's schematic
- [ ] Move the serial protocol and conversions into their own tested classes
- [ ] Touch-screen layout; encoder and buttons
- [ ] Matrix SetPins support
- [ ] Tube Desk (basing diagrams, wiring lists, data-sheet links)
- [ ] Limits and confirmation above about 450 V
- [ ] Windows installer / ready-to-run download

### Hardware
- [ ] Matrix relay board: requirements and relay choice
- [ ] Matrix relay board: schematic and PCB
- [ ] Matrix relay board: build and 1 kV insulation test
- [ ] Dummy-load set: design, build, test
- [ ] Plain case: design (stock enclosure, drilling templates, room for an optional Pi and display)
- [ ] Plain case: build guide, plus an addendum for fitting the Pi and display

### Testing on a uTracer6
- [ ] Capture Dekker's own program on the serial line and compare
- [ ] Dummy loads, low voltage
- [ ] Dummy loads, up to 1000 V
- [ ] Reference tube against Dekker's program

### Documentation
- [x] Simulator guide (`tools/utracer_sim/README.md`)
- [x] Developer manual, first edition (`docs/DEVELOPER.md`); grows with stage 5
- [ ] Dummy-load build guide
- [ ] Matrix board build guide
- [ ] User manual

### Other uTracers
- [ ] Every test against the uTracer3 profile as well
- [ ] Find out whether the uTracer NXT uses the same protocol
- [ ] Test builds for uTracer3 owners

### Later ideas
- [ ] Transistor curve tracing (see `PLAN.md`)
