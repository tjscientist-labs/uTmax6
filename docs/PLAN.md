# The uTmax6 plan

This is how uTmax6 is being built, and why in this order. What is finished, and
what is next, is tracked in [`PROGRESS.md`](PROGRESS.md).

## Aim

A uTmax that a uTracer6 owner can trust at 1000 V, on Linux, a Raspberry Pi or
Windows, whose code is tested and commented, and whose every change is explained.
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

### 4. Bug fixes, one per commit
Each with its own test. The list so far:

- 425 V limit on sweeps with a non-zero delay (the hold path); should follow `VaMax`.
- Discharge (End Measurement, command `30`) sent only by a 20-second idle
  heartbeat; send it on Stop and at the end of every sweep. (After Dean Jenkins.)
- `cal.txt` rewritten every time the port opens at start-up. (Dean Jenkins.)
- `~/uTmax_files` never checked or created; calibration silently not saved.
  (Dean Jenkins.)
- `data.csv` header writes `gm1Del` twice. (Dean Jenkins.)
- `Style.qss` and the log file use the current folder.
- New `cal.txt` defaults: `VaMax=300`, `COM=COM1`.
- Smaller items: integer division in `Update_adc_scales()`, a dead assignment in
  `GetVg()`, no-op `d.remove(i,0)` calls in `readData()`, unchecked `QFile::open()`
  results, an uninitialised `size`, a switch fall-through.
- Check the uTracer6 conversion constants (1 MΩ dividers, negative-rail and supply
  dividers) against Dekker's schematic.

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

## Hardware alongside the software

- **A new Matrix relay board rated for 1000 V.** Nick Barton's board is no longer
  sold. The replacement keeps his SetPins protocol, so it works with his software
  as well as uTmax6. Relays are switched only while the reservoirs are discharged
  (cold switching), and need at least 1.5 kV across open contacts and from coil to
  contact. Schematic, PCB files and a parts list will be published as open hardware,
  crediting his design.
- **Dummy loads** for stage 7, published with values, parts and ratings.

## Later ideas (not started)

- **Transistor curve tracing.** The uTracer's negative grid supply suits JFETs and
  depletion-mode devices; with Dekker's positive-grid extension (0 to +100 V with
  grid-current measurement) enhancement MOSFETs look possible too. Bipolar
  transistors need stepped base current rather than voltage, perhaps through a
  series resistor from the positive grid supply with the software converting.
  Worth exploring once the tube side is solid.
