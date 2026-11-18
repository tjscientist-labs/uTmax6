# Changes from uTmax 3.01

The GPL asks a modified program to say what was changed and when. This file does
that in plain words, newest first. The first commit in this repository is Nick
Barton's uTmax 3.01 exactly as he published it, so `git diff <first commit>` shows
every line that differs. Each entry gives the reason, and credits Dean Jenkins
where the fix comes from his fork.

## 2026-11-18

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
