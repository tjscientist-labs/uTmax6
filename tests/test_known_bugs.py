"""Known bugs in uTmax 3.01, recorded as tests before they are fixed.

Each test states what uTmax 3.01 does. When a bug is fixed, its test moves to a
test of the correct behaviour in the same commit, and the bug is listed below.

Open: none recorded as a test yet. The list of known problems is in
docs/PLAN.md (stage 4) and docs/DEVELOPER.md (section 11).

Fixed:
- 2026-11-20  Crash at start-up (std::out_of_range) with a tube library that
              has no usable tube. Now tests/test_tube_library.py.
- 2026-11-20  New cal.txt: COM=COM1 on Linux, a style file that doesn't exist;
              theme images relative to the current folder. Now
              tests/test_new_install.py.
- 2026-11-20  ~/uTmax_files never created (file dialog, settings never saved);
              Style.qss and logs in the current folder. Now
              tests/test_app_paths.py.
- 2026-11-20  cal.txt rewritten at every start, reconnect and exit; Index=
              (tube last used) lost at start-up. Now tests/test_cal_file.py.
- 2026-11-20  End Measurement (30, discharge) sent only by the 20 s idle
              heartbeat, not when a sweep ends or is stopped. Now
              tests/test_discharge.py.
- 2026-11-20  Crash (SIGFPE) on the first plotted point when the style sheet
              can't be opened: no pens. Now tests/test_plot_pens.py.
- 2026-11-20  Negative-rail reading: uTracer6 decoded with the -15 V divider
              (showed -17.7 V for -85 V); uTracer3 integer division (24 for
              24.5). Now tests/test_neg_rail.py.
- 2026-11-18  Calibration values uninitialised; new-install cal.txt without
              Vg0/Vgm; unreadable values became 0. Now
              tests/test_calibration_defaults.py.
- 2026-11-18  Delayed sweeps stopped above 425 V on a uTracer6; no backstop on
              sweeps without a delay. Now tests/test_voltage_limit.py.
- 2026-11-18  Positive-grid board's 42-character replies never accepted (uTmax
              timed out and reconnected); status 91 not seen as a current limit.
              Now tests/test_reply_format.py and UTmaxWithPositiveGridBoard in
              tests/test_utmax_smoke.py.
- 2026-11-18  Positive grid on a uTracer6 sent as FFFF (+100 V), heater word lost.
              Now tests/test_grid_word.py; 3.01 reproduced in
              tests/repro/positive_grid_word.cpp.
"""
