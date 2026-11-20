# The software uTracer

`utracer_sim.py` pretends to be a uTracer on a serial port. A host program such as
uTmax connects to it exactly as it would to the real instrument, and the
simulator answers the way Ronald Dekker's firmware does. Use it to:

- run and learn uTmax without a uTracer;
- test changes to uTmax without risking hardware;
- see exactly what a host program sends, command by command;
- check how a host copes with faults: no reply, a corrupted reply, a current-limit trip.

It is plain Python 3 with no extra packages. It runs on Linux (including the
Raspberry Pi) and macOS. Windows support (through a virtual COM-port pair) is
planned.

## Quick start

```sh
python3 tools/utracer_sim/utracer_sim.py --load r:10k --log /tmp/sim.jsonl
```

It prints the port it made:

```
uTracer simulator: profile=utracer6, load=resistor(anode=10000 ohm)
serial port: /tmp/utracer-sim -> /dev/pts/4
```

Point uTmax at `/tmp/utracer-sim`: set `COM=/tmp/utracer-sim` in
`~/uTmax_files/cal.txt`, or choose it in uTmax's Debug dialog. Every command uTmax
sends is then written to `/tmp/sim.jsonl`, one line each, decoded:

```json
{"n": 2, "rx": "1000E3000000000000", "cmd": "measure",
 "decoded": {"Va_reservoir_V": 237.17, "Vs_reservoir_V": 0.0, "Vg_V": 0.0, "Vf_Vrms": 0.0},
 "measured": {"Ia_mA": 21.7567, "Is_mA": 0.0, "Va_tube_V": 217.57, "gain_a": 20,
              "gain_s": 200, "averaging": 1, "status": "10"},
 "tx": "1001A200150000000000E30000016B028B0407"}
```

(A real line: anode code `00E3` is 237 V on the reservoir with the draft
uTracer6 profile; less the 19.5 V supply and the sense-resistor drop, the 10 kΩ
load sees 217.6 V and draws 21.8 mA. The auto-range picked a gain of 20. The
screen has no current, so its auto-range went to the top gain, 200.)

Stop it with Ctrl-C.

## Options

| Option | Meaning |
|---|---|
| `--profile utracer6` | Which hardware to model (`utracer6` or `utracer3`; files in `profiles/`). |
| `--load SPEC` | The device under test, see below. Default `r:10k`. |
| `--link PATH` | Where to put the serial port. Default `/tmp/utracer-sim`. |
| `--log FILE` | Append one JSON line per command. |
| `--reply-delay MS` | Delay before a measurement reply. Default 30 ms. |
| `--no-reply-after N` | Fault: stop replying after N commands (tests a lost connection). |
| `--garble-every N` | Fault: corrupt every Nth reply. |
| `--trip` | Fault: report a current-limit trip on every measurement. |
| `--bad-echo-on N` | Fault: the last character of command N is echoed wrongly (an `X`). The simulator still acts on the command and replies, as a uTracer would if the line corrupted the echo on its way back. Logged as `"fault": "bad echo"`. |
| `--supply-volts V` | The supply (power brick) voltage, instead of the profile's 18.5 V / 19.5 V. For a weak or sagging brick: uTmax scales the heater to the supply it reads. |
| `--grid-board` | Fit Dekker's positive-grid extension board (uTracer6): status bit 7 set, 42-character replies. Without it, a positive grid word is ignored and logged as a warning. |

### Loads

| Spec | What it is |
|---|---|
| `open` | Nothing connected. |
| `r:10k` | A resistor from anode to cathode. |
| `r:10k,s:47k` | Plus a resistor from screen to cathode. |
| `diode:0.7,1k` | A diode drop of 0.7 V, then 1 kΩ. |
| `triode:12AX7` | Norman Koren's triode model with published 12AX7 parameters. |

The resistor and diode loads match the physical dummy loads used for the first
hardware checks, so a simulated run and a bench run of the same load can be
compared directly.

To see the curves a load should give, draw them as an SVG (standard library only):

```
python3 tools/utracer_sim/plot_curves.py --load triode:12AX7 --out 12ax7.svg
python3 tools/utracer_sim/plot_curves.py --load r:100k --va-max 300 --grids 0
```

That picture is what uTmax should draw when it sweeps the simulator with the same
load, so it is the reference to compare a run against.

## How it works

The uTracer protocol is simple (Dekker, dos4ever.com, uTracer3 log §15):

- The host sends commands of 18 hex characters: a 2-digit command code, then four
  16-bit words (anode, screen, grid, heater).
- The uTracer echoes **every character** as it arrives.
- Two commands get a 38-character reply after the echo: `10` (measure) and `50`
  (read all ADC channels). The others (`00` settings, `20` apply and hold,
  `30` end measurement and discharge, `40` heater) get only the echo.

The simulator does the same. For a measurement it:

1. converts the command's codes to volts using the **hardware profile** (divider
   resistors, sense resistors, grid DAC range, supply voltage);
2. works out the voltage the tube actually sees: reservoir voltage, minus the
   supply (the cathode sits at the supply voltage), minus the drop across the
   current-sense resistor;
3. asks the **load** what current flows at those voltages, and solves the small
   loop between current and sense-resistor drop;
4. checks the current limit the host set (the compliance byte goes straight into
   the PIC's comparator reference: Dekker's example `8F` with 18 Ω is 200 mA);
5. picks the amplifier gain as the firmware's auto-range would, and encodes the
   reply in the order uTmax reads it: status, anode current (after and before the
   amplifier), screen current (after and before), anode voltage, screen voltage,
   supply voltage, negative rail, anode gain, screen gain.

### Why the hardware numbers are in a separate file

The simulator is there to catch mistakes in the host program. If it borrowed its
conversion factors from uTmax it would agree with uTmax's mistakes. So every
hardware constant lives in `profiles/<model>.json` with a note of where it came
from. Both profiles use Dekker's published values: the uTracer3 profile from his
protocol description (it shows that uTmax's default anode divider reads about
2.4 % low before calibration), the uTracer6 profile from his uTracer6
construction log (1 MΩ / 4.7 kΩ dividers, 4.7 Ω sense resistors, 16-bit
sign-and-magnitude grid word, and the −85 V rail through 200 kΩ / 4.7 kΩ as the
negative rail in the reply).

## Tests

From the repository root:

```sh
python3 -m unittest discover -s tests -v
```

`tests/test_simulator.py` checks the simulator against Dekker's formulas and over
a real pseudo-terminal. `tests/test_utmax_smoke.py` starts a built uTmax with no
window against the simulator and checks what it sends; give it the program with
`UTMAX_BIN=build/release/uTmax`.

## Limits (for now)

- Reservoir droop during a pulse is not modelled.
- Auto-averaging uses the rule uTmax assumes; the firmware's real rule is to be checked.
- Nick Barton's own firmware commands (`51`–`59`, for his uTracer3 replacement
  chip) are logged as `unknown` and not answered.
- Linux and macOS only.
