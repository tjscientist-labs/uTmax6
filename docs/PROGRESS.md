# Progress

What has been done on uTmax6, and what comes next. Newest first in each list. The
reasons behind the order are in [`PLAN.md`](PLAN.md); every code change is
described in [`CHANGES.md`](../CHANGES.md).

**Last updated:** 2026-11-18

## Done

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

- The software uTracer (simulator).
- Design of a 1000 V Matrix relay board to replace Nick Barton's.
- Dummy-load designs for the first hardware checks.

## Planned

### Software
- [ ] Simulator: Dekker's protocol, a tube model, fault injection, command log
- [ ] Tests that record 3.01's present behaviour
- [ ] Fix the 425 V limit on delayed sweeps
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

### Testing on a uTracer6
- [ ] Capture Dekker's own program on the serial line and compare
- [ ] Dummy loads, low voltage
- [ ] Dummy loads, up to 1000 V
- [ ] Reference tube against Dekker's program

### Documentation
- [ ] Simulator guide
- [ ] Dummy-load build guide
- [ ] Matrix board build guide
- [ ] User manual

### Later ideas
- [ ] Transistor curve tracing (see `PLAN.md`)
