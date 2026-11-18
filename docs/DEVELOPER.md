# uTmax6 developer manual

For anyone who wants to read, change, test or extend uTmax6. It explains how the
program is built, how it works inside, how it is tested, and how changes are
made. It grows as the code is reorganised (plan stage 5); the line numbers below
refer to the code as imported from uTmax 3.01 and will drift as it changes.

Users want the user manual instead (to be written; see `PLAN.md`, stage 8).

**Contents:** 1. Repository layout · 2. Building · 3. How uTmax works inside ·
4. The uTracer protocol · 5. Conversions and calibration · 6. Files on disk ·
7. The software uTracer · 8. Tests · 9. Making a change · 10. Supporting each
uTracer model · 11. Known problems · 12. Glossary

---

## 1. Repository layout

| Path | What it is |
|---|---|
| `src/` | The program: Nick Barton's uTmax 3.01 source plus uTmax6 changes. Qt 6, C++. |
| `src/uTmax.pro` | qmake project file. |
| `src/Themes/`, `src/MoreThemes/` | Qt style sheets (`.qss`) and their images. |
| `src/data.csv` | The tube library shipped with uTmax. |
| `src/GPL.txt` | Nick Barton's copy of the GPL (same as `LICENSE`). |
| `tools/utracer_sim/` | The software uTracer (section 7). |
| `tools/hooks/commit-msg` | Checks every commit message (section 9). |
| `tests/` | Python tests (section 8). `tests/repro/` holds small programs that reproduce a bug exactly. |
| `docs/` | This manual, `PLAN.md`, `PROGRESS.md`. |
| `CHANGES.md` | Every change to the program, in plain words. |
| `.github/workflows/build.yml` | Automatic builds and tests on every push. |

The first commit (`fbed6d3`) is uTmax 3.01 byte for byte. `git diff fbed6d3 -- src/`
shows every change ever made to the program.

## 2. Building

See [`BUILDING.md`](../BUILDING.md) for Linux, Raspberry Pi and Windows. For
development, build out of tree and in debug mode:

```sh
mkdir -p build-debug && cd build-debug
qmake6 ../src/uTmax.pro CONFIG+=debug
make -j"$(nproc)"
./debug/uTmax
```

uTmax writes its debug output with `qDebug()`. Run it from a terminal to see it.
It runs without a screen with `QT_QPA_PLATFORM=offscreen`, which is how the
end-to-end test runs it.

## 3. How uTmax works inside

### 3.1 The pieces

| File | Class | What it does |
|---|---|---|
| `main.cpp` | — | Loads `Style.qss` from the current folder, opens the main window. |
| `mainwindow.cpp` (≈4,000 lines) | `MainWindow` | Nearly everything: the serial link, the state machine, sweeps, conversions, calibration file, tube library, saving data. Plan stage 5 splits it. |
| `plotTabWidget.cpp` | `PlotTabWidget` | One plot tab: draws curves with QCustomPlot. |
| `dr_optimize.cpp` | `dr_optimize` | Fits tube models (diode, triode, Koren pentode and others) to measured points with GSL's Nelder-Mead simplex (`gsl_multimin_fminimizer_nmsimplex`, around line 860). |
| `kb_optimize.cpp`, `curvefit.cpp` | — | Older fitting code; **not compiled** (not in `uTmax.pro`). |
| `cal_dialog.cpp` | `Cal_Dialog` | The calibration sliders. |
| `cal_wizard.cpp` | `Cal_Wizard` | The step-by-step calibration wizard. |
| `options_dialog.cpp` | `options_dialog` | Options: uTracer6 mode, averaging, current limit, heater, style. |
| `debug_dialog.cpp` | `debug_dialog` | Serial-port choice, ping, read ADC, firmware log. |
| `datasavedialog.cpp` | `DataSaveDialog` | Save-data options. |
| `grid_word.h` | — | `utracer6GridWord()`: the uTracer6 grid word, sign and magnitude (uTmax6). |
| `qcustomplot.*` | — | QCustomPlot 2.1.0 (third-party plotting library). `qcustomplot_131.*` is an older copy, not compiled. |

### 3.2 The timer and the state machine

uTmax has no worker thread. A `QTimer` fires every **250 ms** (`TIMER_SET` in
`mainwindow.h`) and calls `MainWindow::RxData()` (connected around line 805). Each
tick, `RxData()`:

1. if the port is closed, tries to reopen it every 10 s (`RETRYTIME`);
2. calls `RxPkt()` to see whether the expected reply has fully arrived, timed out
   (`RXTIMEOUT`), or doesn't match the command echo (`RXINVALID`);
3. on an idle tick, counts down the 20 s heartbeat (`HEARTBEAT`) and sets
   `sendPing`;
4. runs one step of the state machine: `switch (status)` (around line 940).

The states (`Status_t`, `mainwindow.h` around line 278):

| State | Meaning |
|---|---|
| `Idle` | Nothing running. `Idle_f()` sends any pending request: ADC read, heartbeat, version, calibration, start of a sweep. |
| `wait_adc`, `rxadc`, `WaitPing` | Waiting for a reply to `50` or the heartbeat `30`. |
| `Heating_wait00`, `Heating_wait_adc`, `Heating` | Heater warm-up before a sweep. |
| `Sweep_set`, `Sweep_adc` | One sweep point: send `10`, wait for its reply, store it. |
| `hold_ack`, `hold` | Sweep with a non-zero delay: apply with `20`, wait, then measure. |
| `HeatOff` | Sweep finished or stopped. |
| `apply`, `apply_measure`, `apply_response`, `apply_end` | Calibration wizard steps. |
| `readcal`, `readversion`, `writeChipCal`, `erase_flash`, `pgm_flash` | Nick Barton's replacement firmware chip only. |

The control flags (`sendPing`, `sendADC`, `startSweep`, `stop`, ...) are set by
the user interface and read on the next tick, so a button press takes effect
within 250 ms.

### 3.3 A sweep, step by step

1. **Start** (`on_Start_clicked()`, line 3344) checks the entries (`VaMax`, grid
   range), then sets `startSweep`.
2. `Idle_f()` saves the control state, builds the list of points
   (`CreateTestVectors()`, line 3364), clears the data store, and sends the
   settings command `00`: current limit, averaging, screen gain, anode gain.
3. If the heater isn't hot: `50` (read the supply), then `40` (heater on), then a
   countdown (`Heating_f()`).
4. For each point: `Sweep_Set()` checks the anode and screen volts against the
   backstop (`hvWithinLimit()`, `src/voltage_limit.h`: `VaMax`, capped at 425 V
   on a uTracer3 and 1000 V on a uTracer6; a trip stops the sweep with "Internal
   Error: Va or Vs excessive"), converts the volts to codes (`GetVa()`, `GetVs()`,
   `GetVg()`, `GetVf()`) and sends `10`. With a delay set, it sends `20` (apply
   and hold) first, and `Hold()` makes the same check before the `10`.
5. `Sweep_ADC()` reads the reply (`saveADCInfo()`, line 2195, then `GetReal()`,
   line 2217), checks for a current-limit trip (status `11`), and stores the point
   (`StoreData()`, line 1805).
6. After the last point: optional model fit (`dr_optimize::Optimize()`), re-plot
   (`RePlot()`, line 1763), auto-save, then `End()` sends a final `10` at 4 V on
   anode and screen and the heater off or held. The discharge command
   `30` is sent only by the next idle heartbeat (a known problem, section 11).

### 3.4 Receiving

`readData()` (line 816) is connected to the port's `readyRead` signal. It appends
incoming characters to `RxString`, drops spaces, CR and LF, and skips anything from
`<` to the end of the line (comments from Nick Barton's firmware). `RxPkt()` (line
844) waits until `RxString` is exactly `rxLen` characters (echo + reply), checks
that it starts with the command sent, and hands back the reply. For a result
string (commands `10` and `50`) it first reads the status byte: with bit 7 set
(positive-grid board fitted) it waits for 42 characters instead of 38
(`resultLength()` in `src/reply_format.h`; uTmax 3.01 always waited for 38). If
more characters arrive than expected, the length never matches and the request
times out.

## 4. The uTracer protocol

Ronald Dekker's protocol
([uTracer3 log §15](http://dos4ever.com/uTracerlog/tubetester2.html#protocol);
uTracer6 additions in the
[uTracer6 log §8–14](https://www.dos4ever.com/uTracerlog6/tubetester6.html)).
9600 baud, 8N1, upper-case hex.

**Command:** 18 characters, a 2-digit code then four 16-bit words. The uTracer
echoes every character.

| Code | Words | Reply |
|---|---|---|
| `00` | `0000` `0000` then four bytes: compliance, averaging, screen gain, anode gain | echo |
| `10` | anode, screen, grid, heater | echo + 38 |
| `20` | anode, screen, grid, heater (apply and hold) | echo |
| `30` | ignored (end measurement: discharge, grid to 0) | echo |
| `40` | heater in the last word | echo |
| `50` | ignored (read every channel) | echo + 38 |

- **Compliance byte:** written to the PIC's `CVRCON` register. Bit 5 (VRR) picks the
  range; bits 3–0 the ladder tap. Vref = tap/24 · 5 V (VRR = 1) or
  5/4 + tap/32 · 5 V (VRR = 0); the limit is Vref / Rsense. `8F` with 18 Ω is 200 mA.
- **Averaging byte:** 1–32 samples, or `40` for auto.
- **Gain bytes:** PGA113 codes 0–7 = ×1, 2, 5, 10, 20, 50, 100, 200; `08` = auto.

**Reply:** 38 characters: status (`10` OK, `11` current limit), then the words Ia
after the amplifier, Ia before it, Is after, Is before, Va, Vs, supply, negative
rail, then the anode and screen gain bytes actually used. Currents are **sums** of
the averaged samples; the host divides.

**uTracer6 with the positive-grid board:** status has bit 7 set (`90`, `91` on a
current limit) and a 4-character grid-current word (ADC input AN12) is appended:
42 characters. The other fields keep their places. Test the status with
`statusIsCurrentLimit()` (`src/reply_format.h`), never by comparing with `"11"`.

**Nick Barton's firmware chip** (uTracer3 only) adds two-character commands
`51`–`59` (calibration on the chip, version, debug, firmware update). uTmax sends
them only when `Firmware` in `cal.txt` is above 100.

**There is no checksum.** See `PLAN.md`, stage 4, for the checks uTmax6 adds on the
host side.

## 5. Conversions and calibration

| Quantity | uTracer3 (Dekker) | uTracer6 (Dekker) | uTmax 3.01 |
|---|---|---|---|
| Anode/screen reservoir | V = 0.35 n (482k / 6k81) | V = 1.0448 n (1M / 4k7) | `adc_scale.Va` = (rvtop + RaVal) / RaVal, rvtop = 470k or 1M; `RaVal` from `cal.txt` |
| Tube voltage | V_HV − 18.5 V | V_HV − 19.5 V | V_HV − Vsupply − Ia·Rs + 0.5 V − 0.75 V (uTracer3 diode and darlington, applied on the uTracer6 too) |
| Current | n = 3683 · Ia · gain · N (18 Ω) | n = N · gain · Rs · 1023/5 · I (Rs = 4.7 Ω) | `GetReal()` |
| Supply | 0.0234 n | 0.0538 n | `adc_scale.Vsu` |
| Grid | 10-bit PWM, −50/1023 n | 15-bit magnitude at 100 V, bit 15 = positive | `GetVg()` (see section 11) |
| Heater | n = 1023 (Vf/Vsupply)² | same | `GetVf()` |

Calibration multiplies each conversion by a factor near 1 from `cal.txt`
(`Va`, `Vs`, `Ia`, `Is`, `Vsu`, `Vn`, grid offset `Vg0` and gain `Vgm`), set with
the sliders or the wizard.

## 6. Files on disk

Everything lives in `~/uTmax_files/` (the folder isn't created by uTmax 3.01):

- **`cal.txt`**: `key=value` lines. `COM` (serial port), the calibration factors,
  `RaVal`, `VaMax`, `IaRsense`, `IsRsense`, `VgMax`, `Firmware`, `uTracer6`,
  `heater`, `SavePath`, `Style`, `Sort`, `Index`. Created with defaults if missing
  (`ReadCalibration()`, around line 2330); rewritten by `SaveCalFile()`.
- **`data.csv`**: the tube library, one tube per line. Columns, in order: `ID`,
  socket pins for `A, G2, G1, G1b, Cathode, G3, Heater, Heater`, heater volts,
  SPICE model, the sweep ranges (`VaStart`…`VgStep`), limits and model parameters
  (`Vca` … `Power`), emission test (`EmmVa`, `EmmVs`, `EmmVg`), transconductance
  (`gm1Typ`, `gm1Del`, `gm2Typ`, `gm2Del`), heater time, time, uses, `Va=Vs`. The
  first line is a header and is skipped on reading.
- **`Style.qss`**: read from the **current folder** at start-up (section 11).
- **Log file** (when firmware debugging is on): also in the current folder.

## 7. The software uTracer

`tools/utracer_sim/` — full guide in its
[`README.md`](../tools/utracer_sim/README.md). In short:

- `utracer_sim.py`: the device. Opens a pseudo-terminal, links it to
  `/tmp/utracer-sim`, echoes, decodes each command, answers `10` and `50`, logs
  JSON.
- `hardware.py`: volts ↔ codes, driven by a profile.
- `profiles/utracer3.json`, `profiles/utracer6.json`: every hardware constant,
  each with its source.
- `loads.py`: open, resistor, diode, Koren triode.

To add a hardware model, copy a profile, fill in every constant with its source,
and add tests against the maker's published figures. To add a load, add a class
with `currents(va, vs, vg)` returning anode and screen amps, and a `from_spec()`
entry.

## 8. Tests

```sh
python3 -m unittest discover -s tests -v                       # simulator tests
UTMAX_BIN=build/release/uTmax python3 -m unittest discover -s tests -v   # plus end-to-end
```

| File | What it checks |
|---|---|
| `tests/test_simulator.py` | Conversions against Dekker's formulas; loads; the protocol over a real pseudo-terminal; the positive-grid board; fault injection. |
| `tests/test_known_bugs.py` | Known bugs, recorded as they are; a fixed bug's test moves to a test of the correct behaviour. |
| `tests/test_grid_word.py` + `tests/cpp/test_grid_word.cpp` | The uTracer6 grid word: C++ checks of `src/grid_word.h` against Dekker's formula, then its commands decoded by the simulator. Needs g++. |
| `tests/test_reply_format.py` + `tests/cpp/test_reply_format.cpp` | Result-string length and status byte: C++ checks of `src/reply_format.h` (38 or 42 characters; current limit `11` or `91`), and that the simulator's replies agree. |
| `tests/test_voltage_limit.py` + `tests/cpp/test_voltage_limit.cpp` | The anode/screen backstop: C++ checks of `src/voltage_limit.h` for both models, the user's maximum, the 1 V slack and NaN; and that no fixed 425 V is left in `mainwindow.cpp`. |
| `tests/test_sweeps.py` (+ helper `tests/utmax_run.py`) | Whole sweeps of the real program against the simulator through the test hook: a uTracer6 sweep into 100 kΩ (voltages received, currents reported, heater on then off), a delayed sweep to 600 V (the 425 V fix), the same sweep with the positive-grid board, and a sweep above `VaMax` refused with an entry error. |
| `tests/test_calibration_defaults.py` | Through the test hook: a `cal.txt` with no calibration factors, and one with unreadable ones, still sweep correctly (anode and grid voltages, currents) and are written back with the defaults; a new install's `cal.txt` has `Vg0`/`Vgm` and none of the obsolete grid keys. |
| `tests/test_utmax_smoke.py` | Runs the built program with no window against the simulator and checks what it sends, on a uTracer6 with and without the positive-grid board. |

The GitHub workflow runs all of them on Linux x64 and arm64 after building.

### The test hook

`uTmax --test-sweep <file>` (`src/test_hook.cpp`) runs one sweep the way a
person would, then quits. The file holds `key=value` lines: `out` (report path,
required), `tube` (as shown in the tube list), `VaStart` `VaEnd` `VaSteps`,
`VsStart` `VsEnd` `VsSteps`, `VgStart` `VgEnd` `VgSteps`, `Vh`, `HtrSecs`
(heater warm-up, seconds), `Delay` (seconds; non-zero uses the hold command) and
`timeout`. The hook waits until the port is open and uTmax is idle, fills in the
fields, presses Start, and writes a JSON report: `result` (`done`,
`never started`, `timeout`, `no port`), every status-bar message and dialog in
order (dialogs are closed so nothing waits for a click), and every point in the
data store. Without the option uTmax behaves exactly as before.

`tests/utmax_run.py` wraps it: `run_sweep(settings, sim_args=[...])` starts the
simulator, gives uTmax a temporary home with a complete `cal.txt` and a real style
sheet (see section 11 for why both matter), runs the sweep with Qt's offscreen
platform and core dumps off, and returns the report and the simulator's command
log. A typical test checks both: what uTmax reported, and what the simulator
received.

## 9. Making a change

Read [`CONTRIBUTING.md`](../CONTRIBUTING.md) first. The short version:

1. **One change per commit.** A fix, a refactor and a feature are three commits.
2. **Test first.** Add or change a test that shows the behaviour before you change
   it, then change the code and the test together.
3. **The commit message names every file it changes and says why.** The hook
   (`git config core.hooksPath tools/hooks`) refuses one that doesn't.
4. **Update `CHANGES.md`** with any change under `src/`, and `docs/PROGRESS.md` when
   an item moves.
5. **Credit sources.** A fix from Dean Jenkins' fork names his commit
   (`skullandbones/uTmax@<hash>`); protocol facts cite Dekker's page and section.
6. **Push; check the build.** All three platforms must stay green.

## 10. Supporting each uTracer model

- **uTracer3 / 3+** (Dekker firmware): the default when `uTracer6=0`. Keep it
  working; every test should pass against both profiles.
- **uTracer3 with Nick Barton's chip:** `Firmware` ≥ 200 in `cal.txt`. Keep the code
  paths; we cannot test them.
- **uTracer6:** `uTracer6=1`. The main target.
- **uTracer NXT:** not yet known whether it shares the protocol.

## 11. Known problems

Recorded in `docs/PLAN.md` (stage 4), with tests in `tests/test_known_bugs.py`
where they exist. The most serious:

- ~~**Positive grid on a uTracer6**~~ **fixed 2026-11-18**: `GetVg()` now calls
  `utracer6GridWord()` (`src/grid_word.h`). 3.01's behaviour is kept in
  `tests/repro/positive_grid_word.cpp`.
- ~~**42-character replies** with the positive-grid board~~ **fixed 2026-11-18**:
  `RxPkt()` reads the status byte first (`src/reply_format.h`). The grid-current
  word is received but not yet shown.
- ~~**425 V cap on delayed sweeps**~~ **fixed 2026-11-18**: one backstop for every
  measurement, `hvWithinLimit()` in `src/voltage_limit.h`.
- ~~**Calibration values not initialised**~~ **fixed 2026-11-18**: every
  `calData_t` field has a default; a new `cal.txt` has `Vg0`/`Vgm`; unreadable
  values keep their default.
- **Crash (division by zero) on the first plotted point** when the style sheet
  can't be opened: the pen list is never filled.
- **Discharge waits up to 20 s** after a sweep (section 3.3).
- `cal.txt` rewritten on every start; `~/uTmax_files` not created; `Style.qss` and
  the log read and written in the current folder.

## 12. Glossary

| Term | Meaning |
|---|---|
| Reservoir | The capacitor (50 µF on a uTracer6) charged to the anode or screen voltage and switched onto the tube for each ~1 ms pulse. |
| Compliance | The current limit; the pulse is cut short (about 10 µs) when it trips. |
| PGA | Programmable-gain amplifier (PGA113) in front of the ADC for the currents. |
| Count, code, n | A 10-bit ADC or PWM value, 0–1023. |
| Ping | uTmax's name for its idle heartbeat; it actually sends `30`, end measurement. Dekker's "ping" is `50`. |
| Matrix board | Nick Barton's relay board that wires socket pins to functions; uTmax6's 1 kV replacement is described on the project's website, Vol 5. |
