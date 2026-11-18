# uTmax6

**uTmax6 is a fork of Nick Barton's uTmax**, the open-source host program for
Ronald Dekker's [uTracer](https://www.dos4ever.com/uTracer3/uTracer3_pag0.html)
vacuum-tube curve tracers. It aims to make uTmax a dependable program for the
**uTracer6** (Dekker's 1000 V / 1 A model) on **Linux, the Raspberry Pi and
Windows**, with the code tested, commented and documented well enough that
anyone can follow every change and repeat every step.

> **Status: early work. The uTracer6 mode is not yet tested on hardware.**
> The code in this repository is, so far, Nick Barton's uTmax 3.01 with only build
> changes. uTmax's uTracer6 mode was written without a uTracer6 to test against,
> and it has not been checked against one here yet either. Do not rely on it at
> high voltage until the hardware checks in [`docs/PROGRESS.md`](docs/PROGRESS.md)
> are done.

## Where this came from

| | |
|---|---|
| **uTmax** | Written by **Nick Barton** (Black Magic Amplifiers), 2013 onward, GPLv3. Version 3.01 is the starting point of this fork. [bmamps.com: uTracer / uTmax](https://bmamps.com/v01/home/techie-corner/utracer-utmax/) |
| **Dean Jenkins' fork** | **Dean Jenkins** fixed and reworked uTmax 1.3a for Linux in 2017–2018 and documented building it on a Raspberry Pi 4. His fixes that are still missing from 3.01 are being carried into this fork, credited commit by commit. [github.com/skullandbones/uTmax](https://github.com/skullandbones/uTmax) |
| **The uTracer** | Designed by **Ronald Dekker**. The serial protocol and hardware descriptions used here are his. [dos4ever.com](https://www.dos4ever.com/) |

Full credits: [`CREDITS.md`](CREDITS.md). Every change against Nick Barton's
original, with the reason for it: [`CHANGES.md`](CHANGES.md). The first commit in
this repository is his 3.01 source exactly as published, so `git diff` against it
shows everything that has been changed.

## What is planned

The full plan is in [`docs/PLAN.md`](docs/PLAN.md), and what is done so far is in
[`docs/PROGRESS.md`](docs/PROGRESS.md). In short:

1. **A software uTracer.** A simulator that behaves like a uTracer on a virtual
   serial port, so the program can be tested without hardware, including faults.
2. **Tests** that pin down what uTmax does now, before anything is changed.
3. **Bug fixes**, one per commit, each with a test.
4. **Reorganising and commenting** the code, starting with the serial protocol.
5. **New features**: a touch-screen layout, Matrix relay-board support (SetPins),
   a tube lookup with pinouts, and stricter high-voltage limits.
6. **Hardware checks**: dummy loads at low voltage, a comparison against Dekker's
   own program, then real tubes.
7. **A user manual.**

A replacement for Nick Barton's Matrix relay board, rated for the uTracer6's
1000 V, is being designed alongside. It will be published as open hardware.

## Building

See [`BUILDING.md`](BUILDING.md). In short, on Debian, Ubuntu or Raspberry Pi OS:

```sh
sudo apt install qt6-base-dev qt6-base-dev-tools libqt6serialport6-dev libgsl-dev build-essential
mkdir build && cd build
qmake6 ../src/uTmax.pro CONFIG+=release
make -j4
```

Every push is built automatically on Linux x64, Linux arm64 and Windows
(GitHub Actions).

## Documentation

The write-up of the whole project, including the simulator, the dummy loads and
the relay board, is published at
**[test-equipment.fubsypoly.com/utmax6/](https://test-equipment.fubsypoly.com/utmax6/)**.

## Licence

GNU General Public License, version 3 or later, as Nick Barton released uTmax.
See [`LICENSE`](LICENSE). There is no warranty. A uTracer6 stores about 25 J at
1000 V in its reservoir capacitors; treat it accordingly.
