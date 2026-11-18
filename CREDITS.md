# Credits

uTmax6 stands entirely on other people's work. Each of them is credited here,
with links to their own pages.

## Nick Barton — author of uTmax

Nick Barton (Black Magic Amplifiers) wrote uTmax, starting in 2013, and released
it under the GNU GPL. He added uTracer6 support in version 2.3 and released the
64-bit Qt 6 version 3.00 and then 3.01, which is the base of this fork. He also
designed the Matrix pin-switch board, which wires a tube's socket pins
automatically.

- uTmax: <https://bmamps.com/v01/home/techie-corner/utracer-utmax/>
- Matrix pin-switch board: <https://bmamps.com/v01/matrix-pin-switch-board-for-utracer/>
- Source used here: <https://bmamps.com/download/utmax64_QT6.zip>

`src/mainwindow.cpp` and most of the program are Copyright (C) 2013 Nick Barton.

## Dean Jenkins — uTmax 1.3a for Linux

Dean Jenkins maintained a fork of uTmax 1.3a in 2017–2018: a rewritten serial
state machine, bug fixes, Linux build instructions, and in 2021–2024 instructions
for building on a Raspberry Pi 4. Nick Barton's later versions did not take in his
changes. Where uTmax6 carries one of his fixes forward, the commit names his
original commit.

- <https://github.com/skullandbones/uTmax>

## Ronald Dekker — the uTracer

Ronald Dekker designed the uTracer3, uTracer6 and uTracer NXT, and published their
construction, firmware behaviour and serial protocol. uTmax and uTmax6 are host
programs for his hardware; the protocol notes and the simulator in this
repository follow his descriptions.

- <https://www.dos4ever.com/>

## Norman Koren — tube models

The `IaCalc()` tube model in uTmax is derived from Norman Koren's MATLAB work and
is used with his permission, as recorded by Nick Barton in `src/mainwindow.cpp`.

- <https://www.normankoren.com/Audio/Tubemodspice_article.html>

## Libraries

- **Qt 6** (The Qt Company and contributors), LGPLv3.
- **QCustomPlot** 2.1.0 by Emanuel Eichhammer, GPLv3, included as `src/qcustomplot.*`.
  <https://www.qcustomplot.com/>
- **GNU Scientific Library**, GPLv3, linked from the system.

## uTmax6

Fork started in November 2026 by tjscientist
(<https://test-equipment.fubsypoly.com/utmax6/>), with code and documentation
written with the help of Claude (Anthropic).
