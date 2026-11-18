# Working on uTmax6

## Every commit explains itself

The commit history is the change log, so every commit message must be complete:

1. **A summary line** saying what the commit does.
2. **A body** that names **every file the commit changes** and says what changed
   in it and why. If a fix comes from Dean Jenkins' fork, name his commit
   (for example `skullandbones/uTmax@391cb4f`).
3. **`CHANGES.md` updated in the same commit** whenever program code under `src/`
   changes, in plain words, under the date.
4. **`docs/PROGRESS.md` updated** when an item is finished or started.

Example:

```
Fix: stop rewriting cal.txt every time the port opens at start-up

src/mainwindow.cpp: OpenComPort() takes a flag saying whether to save the
  calibration file; SerialPortDiscovery() passes false. Saving the file at
  every start served no purpose (it had just been read) and is a needless
  SD-card write on a Raspberry Pi. After Dean Jenkins,
  skullandbones/uTmax@391cb4f.
src/mainwindow.h: new OpenComPort() signature.
src/debug_dialog.cpp: choosing a port by hand still saves it (passes true).
tests/test_startup.py: new test; fails on 3.01, passes now.
CHANGES.md: entry for this fix.
```

A hook checks rules 2 and 3. Turn it on once in your clone:

```sh
git config core.hooksPath tools/hooks
```

## One change per commit

A bug fix, a refactor and a new feature are three commits, not one. That keeps
each change easy to review and easy to undo.

## Tests first

Before changing behaviour, add or update a test (see `tests/`) that shows the old
behaviour, then change the code and the test together.

## Style

Follow the style of the code around you. New code: Qt types, four-space indents,
and comments that say *why*, not only *what*.
