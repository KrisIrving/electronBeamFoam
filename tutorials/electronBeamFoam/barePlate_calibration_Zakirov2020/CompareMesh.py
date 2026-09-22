#!/usr/bin/env python3
from pathlib import Path
import csv
import re

case = Path(__file__).resolve().parent
old_dir = case / "comparisons" / "preflight_tol1e-3_uniformFine"
new_fz = case / "postProcessing" / "meltPoolDiagnostics" / "fusionZone.csv"
old_fz = old_dir / "fusionZone.csv"
old_summary = old_dir / "run-summary.txt"
new_summary = case / "run-summary.txt"

for p in (old_fz, new_fz, old_summary, new_summary):
    if not p.is_file():
        raise SystemExit(f"Missing {p}")

def last_row(path):
    with path.open(newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        raise SystemExit(f"No rows in {path}")
    return rows[-1]

def summary_scalar(path, pattern, last=True):
    text = path.read_text(errors="replace")
    vals = re.findall(pattern, text)
    if not vals:
        return None
    return float(vals[-1] if last else vals[0])

old = last_row(old_fz)
new = last_row(new_fz)

print("Calibration mesh A/B")
print("  baseline = uniformFine, base cells 1,013,760")
print("  probe    = gradedY,    base cells   570,240 (-43.75%)")

for label, key, scale, unit in (
    ("W", "width_m", 1e6, "um"),
    ("D", "depth_m", 1e6, "um"),
    ("L", "length_m", 1e6, "um"),
    ("V", "volume_m3", 1e9, "mm^3"),
):
    a = float(old[key])*scale
    b = float(new[key])*scale
    rel = 100.0*(b-a)/(a if abs(a) > 1e-30 else 1.0)
    print(f"  {label}: {a:.6g} -> {b:.6g} {unit}   delta={rel:+.3f}%")

old_clock = summary_scalar(old_summary, r"ClockTime = ([0-9.eE+-]+) s")
new_clock = summary_scalar(new_summary, r"ClockTime = ([0-9.eE+-]+) s")
old_cells = summary_scalar(old_summary, r"global cells\s*=\s*([0-9.eE+-]+)")
new_cells = summary_scalar(new_summary, r"global cells\s*=\s*([0-9.eE+-]+)")
old_caps = summary_scalar(old_summary, r"thermal cap hits\s*=\s*([0-9.eE+-]+)")
new_caps = summary_scalar(new_summary, r"thermal cap hits\s*=\s*([0-9.eE+-]+)")

if old_cells is not None and new_cells is not None:
    print(
        f"  final global cells: {old_cells:.0f} -> {new_cells:.0f} "
        f"({100.0*(new_cells-old_cells)/old_cells:+.2f}%)"
    )
if old_caps is not None and new_caps is not None:
    print(f"  last-interval thermal cap hits: {old_caps:.0f} -> {new_caps:.0f}")
if old_clock and new_clock:
    print(
        f"  ClockTime: {old_clock:.1f} -> {new_clock:.1f} s   "
        f"speedup={old_clock/new_clock:.3f}x"
    )

print()
print("Acceptance guide:")
print("  - W/D shifts should remain within the useful mesh-resolution envelope;")
print("  - no new beam-hit/power-conservation problem;")
print("  - global cells and ClockTime should decrease materially.")
