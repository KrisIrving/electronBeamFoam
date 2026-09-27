#!/usr/bin/env python3
from pathlib import Path
import json
import re

case = Path(__file__).resolve().parent
repo = case.parents[2]
short_path = repo / "validation" / "Zakirov2020_decomp_simpleXZ_0p5mm.json"
full_path = repo / "validation" / "Zakirov2020_full_296K_3000mmps_baseline.json"
summary_path = case / "run-summary.txt"

for p in (short_path, full_path, summary_path):
    if not p.is_file():
        raise SystemExit(f"Missing {p}")

short = json.loads(short_path.read_text())
full = json.loads(full_path.read_text())
text = summary_path.read_text(errors="replace")

def last(pattern, cast=float):
    vals = re.findall(pattern, text)
    return cast(vals[-1]) if vals else None

global_cells = last(r"global cells\s*=\s*([0-9.eE+-]+)")
local = re.findall(r"local cells min/max\s*=\s*([0-9]+)\s*/\s*([0-9]+)", text)
imb = last(r"cell imbalance max/mean\s*=\s*([0-9.eE+-]+)")
heavy = last(r"heaviest rank\s*=\s*([0-9]+)", int)
wall = last(r"wall total\s*=\s*([0-9.eE+-]+) s")
steps = last(r"steps\s*=\s*([0-9.eE+-]+)")
clock = last(r"ClockTime = ([0-9.eE+-]+) s")

# final x=0 section
m = re.findall(
    r"section station=\+0\.000000e\+00 m cells=([0-9]+) "
    r"width=([0-9.eE+-]+) m depth=([0-9.eE+-]+) m",
    text
)
central = m[-1] if m else None

print("simpleXZ 1 mm confirmation")
print("  reference short gate = 0.5 mm simpleXZ")
print("  reference developed x=0 = completed 3 mm baseline")
print()

short_imb = short["probe"]["final_interval"]["cell_imbalance_max_over_mean"]
print(f"  0.5 mm final imbalance: {short_imb:.3f}")
if imb is not None:
    print(f"  1.0 mm final imbalance: {imb:.3f}   growth={imb/short_imb:.3f}x")
if global_cells is not None:
    print(f"  1.0 mm final global cells: {global_cells:.0f}")
if local:
    lo, hi = map(int, local[-1])
    print(f"  1.0 mm local cells min/max: {lo} / {hi}   max/min={hi/lo:.3f}x")
if heavy is not None:
    print(f"  heaviest rank: {heavy}")

if steps is not None and wall is not None:
    print(f"  final-interval steps: {steps:.0f}")
    print(f"  final-interval wall: {wall:.3f} s")
    print(f"  wall per step: {wall/steps:.6f} s/step")
if clock is not None:
    print(f"  total ClockTime: {clock:.1f} s")

if central:
    cells, w, d = central
    w_um = float(w)*1e6
    d_um = float(d)*1e6
    full_central = min(
        full["station_sections"],
        key=lambda r: abs(float(r["station_mm"]))
    )
    fw = float(full_central["width_um"])
    fd = float(full_central["depth_um"])
    print()
    print(
        f"  x=0 section: cells={cells}, W={w_um:.3f} um, D={d_um:.3f} um"
    )
    print(
        f"  completed 3 mm x=0: W={fw:.3f} um, D={fd:.3f} um"
    )
    print(
        f"  section delta: dW={w_um-fw:+.3f} um, dD={d_um-fd:+.3f} um"
    )

print()
print("Promotion guide:")
print("  - no instability / MPI failure;")
print("  - imbalance should remain far below the ~3.94 long-track Scotch state;")
print("  - central section should be consistent with the developed-track scale;")
print("  - if passed, promote simpleXZ for future calibration campaign runs.")
