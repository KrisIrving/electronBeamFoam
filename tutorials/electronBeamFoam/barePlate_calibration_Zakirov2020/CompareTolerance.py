#!/usr/bin/env python3
from pathlib import Path
import csv
import re

case = Path(__file__).resolve().parent
old_dir = case / "comparisons" / "preflight_tol1e-4"
new_fz = case / "postProcessing" / "meltPoolDiagnostics" / "fusionZone.csv"
old_fz = old_dir / "fusionZone.csv"
old_summary = old_dir / "run-summary.txt"
new_summary = case / "run-summary.txt"

for p in (old_fz, new_fz):
    if not p.is_file():
        raise SystemExit(f"Missing {p}")

def last_row(path):
    with path.open(newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        raise SystemExit(f"No rows in {path}")
    return rows[-1]

def final_clock(path):
    if not path.is_file():
        return None
    text = path.read_text(errors="replace")
    vals = [float(x) for x in re.findall(r"ClockTime = ([0-9.eE+-]+) s", text)]
    return vals[-1] if vals else None

old = last_row(old_fz)
new = last_row(new_fz)

metrics = [
    ("W", "width_m", 1e6, "um"),
    ("D", "depth_m", 1e6, "um"),
    ("L", "length_m", 1e6, "um"),
    ("V", "volume_m3", 1e9, "mm^3"),
]

print("High-power phase-change tolerance A/B")
print("  baseline epsilonTolerance = 1e-4")
print("  probe    epsilonTolerance = 1e-3")
for label, key, scale, unit in metrics:
    a = float(old[key])*scale
    b = float(new[key])*scale
    rel = 100.0*(b-a)/(a if abs(a) > 1e-30 else 1.0)
    print(f"  {label}: {a:.6g} -> {b:.6g} {unit}   delta={rel:+.3f}%")

t0 = final_clock(old_summary)
t1 = final_clock(new_summary)
if t0 and t1:
    print(f"  ClockTime: {t0:.1f} -> {t1:.1f} s   speedup={t0/t1:.3f}x")
