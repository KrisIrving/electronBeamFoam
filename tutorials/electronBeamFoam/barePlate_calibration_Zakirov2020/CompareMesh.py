#!/usr/bin/env python3
from pathlib import Path
import csv
import json
import re

case = Path(__file__).resolve().parent
repo = case.parents[2]
baseline_path = repo / "validation" / "Zakirov2020_preflight_tol1e-3_uniformFine.json"
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

def summary_scalar(path, pattern):
    text = path.read_text(errors="replace")
    vals = re.findall(pattern, text)
    return float(vals[-1]) if vals else None

old_fz = baseline["fusionZone"]

print("Calibration mesh A/B")
print("  baseline = uniformFine, base cells 1,013,760")
print("  probe    = gradedYSmooth, base cells 696,960 (-31.25%)")

for label, key, scale, unit in (
    ("W", "width_m", 1e6, "um"),
    ("D", "depth_m", 1e6, "um"),
    ("L", "length_m", 1e6, "um"),
    ("V", "volume_m3", 1e9, "mm^3"),
):
    a = float(old_fz[key])*scale
    b = float(new[key])*scale
    rel = 100.0*(b-a)/(a if abs(a) > 1e-30 else 1.0)
    print(f"  {label}: {a:.6g} -> {b:.6g} {unit}   delta={rel:+.3f}%")

old_cells = float(baseline["final_interval"]["global_cells"])
old_caps = float(baseline["final_interval"]["thermal_cap_hits"])
old_clock = float(baseline["clock_time_s"])

new_cells = summary_scalar(new_summary, r"global cells\s*=\s*([0-9.eE+-]+)")
new_caps = summary_scalar(new_summary, r"thermal cap hits\s*=\s*([0-9.eE+-]+)")
new_clock = summary_scalar(new_summary, r"ClockTime = ([0-9.eE+-]+) s")

if new_cells is not None:
    print(
        f"  final global cells: {old_cells:.0f} -> {new_cells:.0f} "
        f"({100.0*(new_cells-old_cells)/old_cells:+.2f}%)"
    )
if new_caps is not None:
    print(f"  last-interval thermal cap hits: {old_caps:.0f} -> {new_caps:.0f}")
if new_clock:
    print(
        f"  ClockTime: {old_clock:.1f} -> {new_clock:.1f} s   "
        f"speedup={old_clock/new_clock:.3f}x"
    )

print()
print("Acceptance guide:")
print("  - W/D shifts should remain within the useful mesh-resolution envelope;")
print("  - no new beam-hit/power-conservation problem;")
print("  - global cells and ClockTime should decrease materially.")
