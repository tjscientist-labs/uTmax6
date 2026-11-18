#!/usr/bin/env python3
"""Draw the anode curves a simulator load produces, as a stand-alone SVG.

These are the curves uTmax should show when it sweeps the simulated uTracer
with the same load, so the picture is a reference to compare a run against.
Standard library only (no matplotlib), so it runs anywhere the simulator does.

    python3 plot_curves.py --load triode:12AX7 --out 12ax7.svg
    python3 plot_curves.py --load triode:12AX7 --va-max 300 --grids 0,-0.5,-1,-1.5,-2,-2.5,-3

Part of uTmax6. GNU GPL v3 or later.
"""

import argparse
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from loads import from_spec  # noqa: E402

W, H = 720, 480                      # whole picture, px
L, R, T, B = 70, 110, 40, 60         # margins: left, right, top, bottom
COLOURS = ["#1f6fb2", "#d1495b", "#2e933c", "#edae49", "#7b4fa0",
           "#00798c", "#8c564b", "#e377c2"]


def nice_step(span, target=6):
    """A round grid step (1, 2 or 5 x 10^n) giving about `target` divisions."""
    raw = span / target
    mag = 10 ** math.floor(math.log10(raw))
    for m in (1, 2, 5, 10):
        if raw <= m * mag:
            return m * mag
    return 10 * mag


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--load", default="triode:12AX7", help="load spec, as for utracer_sim.py")
    ap.add_argument("--va-max", type=float, default=300.0, help="anode sweep end, volts")
    ap.add_argument("--grids", default="0,-0.5,-1,-1.5,-2,-2.5,-3",
                    help="grid voltages, comma-separated")
    ap.add_argument("--out", default="curves.svg")
    a = ap.parse_args()

    load = from_spec(a.load)
    grids = [float(g) for g in a.grids.split(",")]
    steps = 120
    curves = []
    for vg in grids:
        pts = []
        for i in range(steps + 1):
            va = a.va_max * i / steps
            ia, _ = load.currents(va, 0.0, vg)
            pts.append((va, ia * 1000.0))          # mA
        curves.append((vg, pts))

    ia_max = max(p[1] for _, pts in curves for p in pts) or 1.0
    ystep = nice_step(ia_max)
    ytop = ystep * (int(ia_max / ystep) + 1)
    xstep = nice_step(a.va_max)
    pw, ph = W - L - R, H - T - B

    def x(v): return L + pw * v / a.va_max
    def y(i): return T + ph * (1 - min(i, ytop) / ytop)

    out = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" '
           f'font-family="sans-serif" font-size="13">',
           f'<rect width="{W}" height="{H}" fill="#ffffff"/>',
           f'<text x="{W / 2}" y="24" text-anchor="middle" font-size="15" font-weight="bold">'
           f'Simulated anode curves: {load.name}</text>']
    v = 0.0
    while v <= a.va_max + 1e-9:
        out.append(f'<line x1="{x(v):.1f}" y1="{T}" x2="{x(v):.1f}" y2="{T + ph}" stroke="#e3e3e3"/>')
        out.append(f'<text x="{x(v):.1f}" y="{T + ph + 18}" text-anchor="middle">{v:g}</text>')
        v += xstep
    i = 0.0
    while i <= ytop + 1e-9:
        out.append(f'<line x1="{L}" y1="{y(i):.1f}" x2="{L + pw}" y2="{y(i):.1f}" stroke="#e3e3e3"/>')
        out.append(f'<text x="{L - 8}" y="{y(i) + 4:.1f}" text-anchor="end">{i:g}</text>')
        i += ystep
    out.append(f'<rect x="{L}" y="{T}" width="{pw}" height="{ph}" fill="none" stroke="#333"/>')
    out.append(f'<text x="{L + pw / 2}" y="{H - 18}" text-anchor="middle">Anode voltage Va (V)</text>')
    out.append(f'<text transform="translate(20 {T + ph / 2}) rotate(-90)" text-anchor="middle">'
               f'Anode current Ia (mA)</text>')
    for n, (vg, pts) in enumerate(curves):
        c = COLOURS[n % len(COLOURS)]
        path = " ".join(f"{'M' if k == 0 else 'L'}{x(va):.1f},{y(ia):.1f}"
                        for k, (va, ia) in enumerate(pts))
        out.append(f'<path d="{path}" fill="none" stroke="{c}" stroke-width="2"/>')
        ly = T + 14 + 20 * n
        out.append(f'<line x1="{L + pw + 12}" y1="{ly}" x2="{L + pw + 32}" y2="{ly}" '
                   f'stroke="{c}" stroke-width="3"/>')
        out.append(f'<text x="{L + pw + 38}" y="{ly + 4}">Vg {vg:g} V</text>')
    out.append("</svg>")
    with open(a.out, "w") as f:
        f.write("\n".join(out) + "\n")
    print(f"wrote {a.out}: {len(grids)} curves, Ia up to {ia_max:.2f} mA")


if __name__ == "__main__":
    main()
