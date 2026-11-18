# Changes from uTmax 3.01

The GPL asks a modified program to say what was changed and when. This file does
that in plain words, newest first. The first commit in this repository is Nick
Barton's uTmax 3.01 exactly as he published it, so `git diff <first commit>` shows
every line that differs. Each entry gives the reason, and credits Dean Jenkins
where the fix comes from his fork.

## 2026-11-18

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
