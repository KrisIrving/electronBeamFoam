#!/usr/bin/env python3
from pathlib import Path
import csv
import json
import re

case = Path(__file__).resolve().parent
repo = case.parents[2]
baseline_path = repo / "validation" / "Zakirov2020_preflight_tol1e-3_gradedYSmooth_pcg.json"
new_fz = case / "postProcessing" / "meltPoolDiagnostics" / "fusionZone.csv"
new_summary = case / "run-summary.txt"

for p in (baseline_path, new_fz, new_summary):
    if not p.is_file():
        raise SystemExit(f"Missing {p}")

baseline = json.loads(baseline_path.read_text())

with new_fz.open(newline="") as f:
    rows = list(csv.DictReader(f))
if not rows:
    raise SystemExit(f"No rows in {new_fz}")
new = rows[-1]

text = new_summary.read_text(errors="replace")

def last_float(pattern):
    vals = re.findall(pattern, text)
    return float(vals[-1]) if vals else None

print("Calibration pressure-solver A/B")
print("  baseline = PCG + DIC")
print("  probe    = GAMG + DICGaussSeidel")

for label, key, scale, unit in (
    ("W", "width_m", 1e6, "um"),
    ("D", "depth_m", 1e6, "um"),
    ("L", "length_m", 1e6, "um"),
    ("V", "volume_m3", 1e9, "mm^3"),
):
    a = float(baseline["fusionZone"][key])*scale
    b = float(new[key])*scale
    rel = 100.0*(b-a)/(a if abs(a) > 1e-30 else 1.0)
    print(f"  {label}: {a:.6g} -> {b:.6g} {unit}   delta={rel:+.3f}%")

old = baseline["final_interval"]
new_cells = last_float(r"global cells\s*=\s*([0-9.eE+-]+)")
new_pressure = last_float(r"pressure\s*=\s*([0-9.eE+-]+) s")
new_total = last_float(r"wall total\s*=\s*([0-9.eE+-]+) s")
new_clock = last_float(r"ClockTime = ([0-9.eE+-]+) s")
new_caps = last_float(r"thermal cap hits\s*=\s*([0-9.eE+-]+)")

if new_cells is not None:
    print(
        f"  final global cells: {old['global_cells']:.0f} -> {new_cells:.0f} "
        f"({100.0*(new_cells-old['global_cells'])/old['global_cells']:+.2f}%)"
    )
if new_pressure is not None:
    print(
        f"  last-interval pressure wall: {old['pressure_s']:.3f} -> "
        f"{new_pressure:.3f} s   speedup={old['pressure_s']/new_pressure:.3f}x"
    )
if new_total is not None:
    print(
        f"  last-interval wall total: {old['wall_total_s']:.3f} -> "
        f"{new_total:.3f} s   speedup={old['wall_total_s']/new_total:.3f}x"
    )
if new_clock is not None:
    print(
        f"  ClockTime: {baseline['clock_time_s']:.1f} -> {new_clock:.1f} s   "
        f"speedup={baseline['clock_time_s']/new_clock:.3f}x"
    )
if new_caps is not None:
    print(f"  last-interval thermal cap hits: {old['thermal_cap_hits']} -> {new_caps:.0f}")

print()
print("Acceptance guide:")
print("  - W/D/L should remain unchanged within the mesh-resolution envelope;")
print("  - pressure wall time should decrease materially;")
print("  - no increase in global cells or loss of source conservation;")
print("  - no pressure convergence failure or runtime instability.")
