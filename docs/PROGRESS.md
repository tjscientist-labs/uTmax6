# Progress

What has been done on uTmax6, and what comes next. Newest first in each list. The
reasons behind the order are in [`PLAN.md`](PLAN.md); every code change is
described in [`CHANGES.md`](../CHANGES.md).

**Last updated:** 2026-11-18

## Done

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

- Which negative rail the uTracer6 reports (-15 V or -85 V).
- Design of a 1000 V Matrix relay board to replace Nick Barton's.
- Dummy-load designs for the first hardware checks.

## Planned

### Software
- [x] Simulator: Dekker's protocol, a tube model, fault injection, command log
- [x] Simulator: uTracer6 constants checked against Dekker's page
- [ ] Simulator: Windows support; reservoir droop
- [ ] Tests that drive a full sweep (needs a test hook in uTmax)
- [ ] Tests that record 3.01's present behaviour
- [x] **Fix the positive-grid command on the uTracer6**
- [x] **Handle the positive-grid board's 42-character replies**
- [ ] Show the grid current from the extension board
- [ ] Allow a positive grid only when the extension board is fitted
- [ ] Check every command before sending, treat a bad echo as an error, check replies
- [x] **Fix the 425 V limit on delayed sweeps**
- [ ] Send End Measurement (discharge) on Stop and at the end of each sweep
- [ ] Stop rewriting `cal.txt` at every start
- [ ] Create `~/uTmax_files` if missing; load styles and write logs there
- [ ] Sensible new-install defaults for Linux and the uTracer6
- [ ] Smaller fixes (see `PLAN.md`, stage 4)
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
