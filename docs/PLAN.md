# The uTmax6 plan

This is how uTmax6 is being built, and why in this order. What is finished, and
what is next, is tracked in [`PROGRESS.md`](PROGRESS.md).

## Aim

A uTmax that a uTracer6 owner can trust at 1000 V, on Linux, a Raspberry Pi or
Windows, and that keeps working for owners of the earlier uTracer3 and uTracer3+, whose code is tested and commented, and whose every change is explained.
Anyone should be able to repeat any step: build the program, run the simulator,
build the dummy loads, build the relay board, and check the results.

## Ground rules

- **Nick Barton's code is the starting point, not a draft to throw away.** The
  first commit is his 3.01 exactly as published. Every later change is a separate
  commit that says what changed and why, and is listed in `CHANGES.md`.
- **Nothing changes without a test that shows what it did before.** uTmax 3.01's
  uTracer6 mode was written without the hardware, and the hardware here is not
  built yet. Tests are the only safe way to change the code until it is.
- **Credit where it is due.** Fixes taken from Dean Jenkins' fork name his commit.
  Protocol facts cite Ronald Dekker's pages.
- **Say what is untested.** Until the hardware checks are done, the README says
  plainly that the uTracer6 mode is untested.

## Stages

### 1. Repository and builds
Import Nick Barton's 3.01 unchanged; make the project file build from system
libraries on Linux, Raspberry Pi and Windows; build all three automatically on
every push.

### 2. A software uTracer (the simulator)
A program that pretends to be a uTracer on a virtual serial port (a pseudo-terminal
on Linux, a virtual COM-port pair on Windows). It follows Dekker's protocol:
echoes every character, answers measurement and ping commands with the 38-character
result string, and models the tube with a simple equation or a resistor. It can be
told to misbehave: no reply, a garbled reply, a current-limit trip, a reply that
arrives late. It logs every command it receives, which is how the tests see what
uTmax sent. The simulator, how it works and how to use it are published, so other
uTracer owners can use it too.

### 3. Tests that record today's behaviour
Using the simulator: the exact command strings uTmax sends for a given sweep, the
voltages and currents it reports for known replies, its behaviour on Stop, on a
current limit and on a lost connection. These tests are written against 3.01 as
it is, so that later changes cannot alter behaviour without a test failing.

**Test hook (done 2026-11-18):** `uTmax --test-sweep <file>` runs one sweep
with given settings and writes a JSON report (`src/test_hook.cpp`), so tests can
drive whole sweeps of the real program against the simulator
(`tests/test_sweeps.py`, helper `tests/utmax_run.py`).

### 4. Bug fixes, one per commit
Each with its own test. The list so far, most serious first:

- ~~**Calibration values not initialised; the default `cal.txt` lacks the grid
  calibration.**~~ **Fixed 2026-11-18** (defaults in `calData_t`, `Vg0`/`Vgm` in
  a new `cal.txt`, unreadable values keep their default;
  `tests/test_calibration_defaults.py`). Found 2026-11-18 with the test hook. `calData` is never given
  default values and `MainWindow` is created on the stack, so any calibration key
  missing from `cal.txt` takes whatever was in memory (seen: `Va=-89254.6`), and
  3.01 then writes that back to `cal.txt` at the next start. The `cal.txt` that
  3.01 creates on a new install writes obsolete keys (`Vg05`, `Vg1`, `Vg4`,
  `Vg40`) instead of `Vg0` and `Vgm`, so the grid calibration of every new
  install is uninitialised until the calibration wizard is run. With garbage
  anode factors uTmax sent 0 V; it could as easily send full scale.
- ~~**Crash on the first point of a sweep when the style sheet can't be opened.**~~
  **Fixed 2026-11-20** (`src/plot_pens.h`, `tests/test_plot_pens.py`). Found 2026-11-18 with the test hook. The plot pens are built only inside
  `SetStyleSheet()`'s "file opened" branch; with no style file the pen list is
  empty and `PlotTabWidget::plotUpdate()` takes `gpnum % penList->length()`, a
  division by zero (SIGFPE). A new install's style path is
  `<program folder>/style.qss`, which a Linux install doesn't have (fixed
  2026-11-20 with the new-install defaults: the bundled themes are installed).
- ~~**Positive grid on a uTracer6 sends the wrong command.**~~ **Fixed 2026-11-18**
  (`src/grid_word.h`, `tests/test_grid_word.py`). `GetVg()` returns a
  negative number as unsigned; `%04X` prints 8 digits and `snprintf` cuts the
  command at 18 characters. For +1 V the uTracer6 receives grid `FFFF`, which
  Dekker's firmware reads as **+100 V**, and the heater word is lost. The hold
  path uses `sprintf` with no limit and sends 22 characters, throwing the
  protocol out of step. Reproduced in `tests/repro/positive_grid_word.cpp`;
  recorded in `tests/test_known_bugs.py`. Dekker's encoding is magnitude in
  bits 0-14 and bit 15 set for a positive grid.
- **Positive grid allowed without the extension board.** In uTracer6 mode uTmax
  accepts grid voltages up to +VgMax whether or not Dekker's positive-grid board
  is fitted. Add an option for the board (the uTracer6 also reports it: status
  bit 7) and refuse positive grid voltages without it.
- ~~**Positive-grid extension board: replies are 42 characters**~~ **fixed
  2026-11-18.** uTmax 3.01 waited for exactly 38, so with the board fitted it
  never accepted a reply (confirmed end to end with the simulator's
  `--grid-board`: three re-reads in 26 seconds, never idle), and its
  current-limit test missed status `91`. `RxPkt()` now reads the status byte
  first (`src/reply_format.h`). Still to do: show the grid-current word.
- **No checking of what goes on the wire.** Dekker's protocol has no checksum or
  CRC; its only check is the character echo, which uTmax compares but only
  after the uTracer has already acted, and replies are not checked at all (a bad
  hex digit silently becomes 0). A checksum can't be added without changing
  Dekker's firmware, and would not have caught the positive-grid bug anyway
  (uTmax built a wrong command and would have checksummed it). So uTmax6 adds
  host-side checks: (1) every command is checked before sending, exactly 18 hex
  characters and every field inside its range for the uTracer model (**done
  2026-11-20**, `src/command_check.h`; it also stops a heater word beyond 10 bits
  when the heater setting is above the supply); (2) an echo
  that doesn't match stops the sweep and discharges (**done 2026-11-20**); (3) replies are checked for
  hex, a known status byte (**done 2026-11-20**, with the gain codes;
  `checkReply()` in `src/reply_format.h`), and reservoir voltages close to what
  was requested (**still to do**: the tolerance needs readings from real hardware,
  since the reservoir droops under load).
- uTracer3 constants used in uTracer6 mode: uTmax adds the uTracer3's 0.5 V
  diode drop and subtracts its 0.75 V darlington drop; Dekker's uTracer6
  formulas have neither (the uTracer6 switch is a MOSFET).

- ~~425 V limit on sweeps with a non-zero delay (the hold path)~~ **fixed
  2026-11-18**: every measurement is checked against `VaMax`, capped at 425 V
  (uTracer3) or 1000 V (uTracer6), in `src/voltage_limit.h`. The no-delay path,
  which had no backstop, now has the same one.
- ~~Discharge (End Measurement, command `30`) sent only by a 20-second idle
  heartbeat~~ **fixed 2026-11-20**: sent on Stop and at the end of every sweep.
  (After Dean Jenkins.)
- ~~`cal.txt` rewritten every time the port opens at start-up~~ **fixed
  2026-11-20**: written only when its text changes, through a temporary file
  (`src/cal_file.h`); the tube last used is now remembered. (Dean Jenkins.)
- ~~`~/uTmax_files` never checked or created; calibration silently not saved~~
  **fixed 2026-11-20**: created at start-up, with the built-in tube library.
  (Dean Jenkins.)
- ~~`data.csv` header writes `gm1Del` twice~~ **fixed 2026-11-20**. (Dean Jenkins.)
- ~~`Style.qss` and the log file use the current folder~~ **fixed 2026-11-20**:
  both in `~/uTmax_files` (`src/app_paths.h`).
- ~~New `cal.txt` defaults: `COM=COM1`, a style sheet that doesn't exist~~
  **fixed 2026-11-20**: the first USB serial port seen (else `/dev/ttyUSB0` on
  Linux), and the bundled themes installed in `~/uTmax_files/Themes`, starting on
  "Utracer Dark"; theme image paths made absolute (`src/new_install.h`).
  `VaMax=300` kept on purpose for a new install.
- ~~Smaller items~~ **all fixed 2026-11-20**: integer division in
  `Update_adc_scales()` (`src/neg_rail.h`); a read of an unset variable in
  `GetVg()`; no-op `d.remove(i,0)` calls in `readData()`; unchecked
  `QFile::open()` results (one of them hid a start-up crash with an empty tube
  library, fixed separately); an uninitialised `size` in the optimiser's debug
  output; a deliberate switch fall-through now marked `[[fallthrough]]`; the
  idle heartbeat's "Send Ping" message.
- uTracer6 conversion constants: Dekker's page confirms 1 MΩ / 4.7 kΩ dividers,
  1 kΩ / 10 kΩ supply divider and 4.7 Ω sense resistors (the simulator's profile
  now uses them). The uTracer6 has two negative rails (-15 V and -85 V); the
  result string carries the -85 V rail, measured through 200 kΩ / 4.7 kΩ
  (Dekker, fig. 7.2). uTmax 3.01 used the -15 V divider; fixed 2026-11-20.

### 5. Reorganise and comment
Move the serial protocol and the voltage/current conversions out of the 4,000-line
main window into their own classes that can be tested on their own. Dean Jenkins'
state machine is the model for the protocol side. Comments are written as each
part is understood, explaining why as well as what.

### 6. New features
- A full-screen touch layout with large controls (for a 13.3-inch screen in the
  uTracer6 case), and the case's rotary encoder and buttons as key presses.
- Matrix relay-board support: send `SetPins` from the tube library before a sweep;
  all-open in manual mode.
- "Tube Desk": basing diagram and wiring list for the selected tube, and links to
  the right page of tube data sheets stored locally.
- Stricter limits and a confirmation step above about 450 V.
- Rename the program to uTmax6 in the window title and files.

### 7. Hardware checks
In this order, each a gate for the next:

1. Simulator only (no hardware).
2. Compare the commands uTmax6 sends with those from Dekker's own Windows program,
   captured on the serial line.
3. uTracer6 into **dummy loads** at low voltage, then up to 1000 V. The dummy-load
   designs are published so anyone can build the same set.
4. A known reference tube, compared with a trace from Dekker's program.

### 8. User manual
Installation, calibration, safety, running sweeps, the tube library, the Matrix
board, and troubleshooting. Its outline and the parts that will not change are
written early; screenshots and step-by-step procedures come after the hardware
checks.

## Which uTracers

- **uTracer3 and uTracer3+** (Dekker's original firmware): supported by uTmax
  3.01 and kept working. Every test runs against both hardware profiles. They
  can't be tested on hardware here, so test builds will be offered to uTracer3
  owners in the uTracer group before a release.
- **uTracer3 with Nick Barton's replacement firmware chip** (commands `51`-`59`,
  calibration stored on the chip): kept working as uTmax 3.01 has it, but
  untestable here without that chip.
- **uTracer6**: the main target.
- **uTracer NXT**: checked 2026-11-20 against Dekker's NXT log §8. Same command and reply strings
  as the uTracer3 (18 and 38 characters, commands 00/10/20/30/40/50, status 10H/11H), but different
  conversions: cathode at ground, 12-bit grid DAC (0 to −120 V), 14.3 Ω sense resistors, 9.76 kΩ / 1 MΩ
  divider (517 V at full scale), −125 V rail via 240 kΩ / 4.7 kΩ, a 1 MΩ minimum load to subtract. uTmax
  needs a third model for it. Simulator profile `utracer_nxt`; how the 12-bit grid number sits in its word
  is to be confirmed with Dekker.

## Hardware alongside the software

- **A new Matrix relay board rated for 1000 V.** Nick Barton's board is no longer
  sold. The replacement keeps his SetPins protocol, so it works with his software
  as well as uTmax6. Relays are switched only while the reservoirs are discharged
  (cold switching), and need at least 1.5 kV across open contacts and from coil to
  contact. Schematic, PCB files and a parts list will be published as open hardware,
  crediting his design.
- **Dummy loads** for stage 7, published with values, parts and ratings.
- **A plain case anyone can build**, alongside the steampunk trunk: a stock
  aluminium or ABS enclosure with 1:1 printable drilling templates (hand tools and
  a drill, no CNC or wood shop); one top plate with the tube sockets, a patch field
  on 1 kV-rated jacks, an HV lamp and a clear cover over the tube; mounting for the
  pin-switch board under the socket plate; an earthed or fully insulated case and a
  cover interlock. It works with any laptop over a USB isolator, and **has room
  for an optional Raspberry Pi and display**, covered by an **addendum to the
  case's build guide**.

## Later ideas (not started)

- **Transistor curve tracing.** The uTracer's negative grid supply suits JFETs and
  depletion-mode devices; with Dekker's positive-grid extension (0 to +100 V with
  grid-current measurement) enhancement MOSFETs look possible too. Bipolar
  transistors need stepped base current rather than voltage, perhaps through a
  series resistor from the positive grid supply with the software converting.
  Worth exploring once the tube side is solid.
