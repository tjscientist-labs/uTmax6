"""Run the real uTmax against the software uTracer, for the end-to-end tests.

    report, sim_log = run_sweep({"VaStart": 30, "VaEnd": 300, ...}, sim_args=["--load", "r:100k"])

cal=None writes a complete cal.txt; a string is written as cal.txt instead
({port}, {style} and {home} are filled in); cal=False writes none (a new install).
The cal.txt uTmax leaves behind is returned in report["cal_after"], and the
file's (inode, mtime in ns) before and after the run in report["cal_stat"].

Starts the simulator on a temporary port, gives uTmax a temporary home folder
with a complete cal.txt (a uTracer6 unless told otherwise) and a real style
sheet, and runs it with Qt's offscreen platform and "--test-sweep" (the test
hook in src/test_hook.cpp). Returns uTmax's JSON report and the simulator's
command log (one dict per command). Your own ~/uTmax_files is never touched.

Needs a built uTmax in UTMAX_BIN. Core dumps are switched off for the uTmax
process, so a crash shows up as a failed test, not as a desktop crash dialog.
"""

import json
import os
import resource
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UTMAX = os.environ.get("UTMAX_BIN")
SIM = os.path.join(ROOT, "tools", "utracer_sim", "utracer_sim.py")


def have_utmax():
    return bool(UTMAX) and os.path.exists(UTMAX)


def _no_core_dumps():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def _stat(path):
    if not os.path.exists(path):
        return None
    st = os.stat(path)
    return (st.st_ino, st.st_mtime_ns)


def run_sweep(sweep, sim_args=(), utracer6=True, va_max=1000, timeout=90, cal=None,
              fresh=False, start_in=None, app_style=None, data_csv=None):
    """fresh=True: a home folder with no uTmax_files at all (a new machine; cal is
    then ignored). start_in: "files" (default), "home" or "elsewhere" (an empty
    folder), the folder uTmax is started from. app_style: text for
    uTmax_files/Style.qss (default: the repository's, which is empty). data_csv:
    text for uTmax_files/data.csv (default: the repository's tube library)."""
    tmp = tempfile.mkdtemp(prefix="utmax-sweep-")
    try:
        home = os.path.join(tmp, "home")
        files = os.path.join(home, "uTmax_files")
        style = os.path.join(files, "theme.qss")
        port = os.path.join(tmp, "utracer")
        rs = "4.7" if utracer6 else "18"
        cal_path = os.path.join(files, "cal.txt")
        if fresh:
            os.makedirs(home)
            cal = False
        else:
            os.makedirs(files)
            shutil.copy(os.path.join(ROOT, "src", "data.csv"), files)
            if data_csv is not None:
                with open(os.path.join(files, "data.csv"), "w") as f:
                    f.write(data_csv)
            if app_style is None:
                shutil.copy(os.path.join(ROOT, "src", "Style.qss"), files)
            else:
                with open(os.path.join(files, "Style.qss"), "w") as f:
                    f.write(app_style)
            shutil.copy(os.path.join(ROOT, "src", "Themes", "Utracer Dark.qss"), style)
        elsewhere = os.path.join(tmp, "elsewhere")
        os.makedirs(elsewhere)
        cwd = {"home": home, "elsewhere": elsewhere}.get(start_in, home if fresh else files)
        if isinstance(cal, str):
            with open(cal_path, "w") as f:
                f.write(cal.format(port=port, style=style, home=os.path.join(tmp, "home")))
        elif cal is None:
            with open(cal_path, "w") as f:
                # Every calibration key uTmax reads (3.01 left missing ones uninitialised).
                f.write(f"COM={port}\nVa=1\nVs=1\nIa=1\nIs=1\nVsu=1\nVg0=0\nVgm=1\nVn=1\n"
                        f"RaVal={'4700' if utracer6 else '6800'}\nVaMax={va_max}\n"
                        f"IaRsense={rs}\nIsRsense={rs}\nVgMax={'100' if utracer6 else '50'}\n"
                        f"Firmware=1\nuTracer6={int(utracer6)}\nheater=0\nStyle={style}\n")
        stat_before = _stat(cal_path)
        report_path = os.path.join(tmp, "report.json")
        settings = dict(sweep)
        settings.setdefault("HtrSecs", 1)
        settings.setdefault("timeout", timeout - 10)
        settings["out"] = report_path
        sweep_file = os.path.join(tmp, "sweep.txt")
        with open(sweep_file, "w") as f:
            f.writelines(f"{k}={v}\n" for k, v in settings.items())

        log = os.path.join(tmp, "sim.jsonl")
        ready = os.path.join(tmp, "ready")
        profile = [] if utracer6 else ["--profile", "utracer3"]
        sim = subprocess.Popen([sys.executable, SIM, "--link", port, "--log", log,
                                "--ready-file", ready] + profile + list(sim_args),
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            for _ in range(100):
                if os.path.exists(ready):
                    break
                time.sleep(0.05)
            env = dict(os.environ, HOME=os.path.join(tmp, "home"), QT_QPA_PLATFORM="offscreen")
            # UTMAX_STDERR=<file> keeps uTmax's debug output, for looking into a failure.
            err = open(os.environ["UTMAX_STDERR"], "w") if os.environ.get("UTMAX_STDERR") \
                else subprocess.DEVNULL
            proc = subprocess.run([os.path.abspath(UTMAX), "--test-sweep", sweep_file],
                                  cwd=cwd, env=env, timeout=timeout,
                                  stdout=subprocess.DEVNULL, stderr=err,
                                  preexec_fn=_no_core_dumps)
        finally:
            sim.terminate()
            sim.wait(timeout=10)
        if not os.path.exists(report_path):
            raise AssertionError(f"uTmax wrote no report (exit status {proc.returncode})")
        with open(report_path) as f:
            report = json.load(f)
        report["exit_status"] = proc.returncode
        if os.path.exists(cal_path):
            with open(cal_path) as f:
                report["cal_after"] = f.read()
        else:
            report["cal_after"] = None
        report["cal_stat"] = (stat_before, _stat(cal_path))
        report["files_after"] = sorted(os.listdir(files)) if os.path.isdir(files) else None
        report["themes_after"] = (sorted(os.path.relpath(os.path.join(d, f), files)
                                         for d, _, fs in os.walk(os.path.join(files, "Themes"))
                                         for f in fs)
                                  if os.path.isdir(os.path.join(files, "Themes")) else None)
        report["tmp"] = tmp
        data_csv = os.path.join(files, "data.csv")
        if os.path.exists(data_csv):
            with open(data_csv) as f:
                report["data_csv_after"] = f.read()
        else:
            report["data_csv_after"] = None
        with open(log) as f:
            commands = [json.loads(line) for line in f]
        return report, commands
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
