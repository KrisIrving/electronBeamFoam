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

print("Calibration PIMPLE pressure-corrector A/B")
print("  baseline = PCG/DIC, nCorrectors=3")
print("  probe    = PCG/DIC, nCorrectors=2")

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
new_steps = last_float(r"steps\s*=\s*([0-9.eE+-]+)")
new_pcorr = last_float(r"pressure correctors\s*=\s*([0-9.eE+-]+)")
new_cells = last_float(r"global cells\s*=\s*([0-9.eE+-]+)")
new_pressure = last_float(r"pressure\s*=\s*([0-9.eE+-]+) s")
new_total = last_float(r"wall total\s*=\s*([0-9.eE+-]+) s")
new_clock = last_float(r"ClockTime = ([0-9.eE+-]+) s")
new_caps = last_float(r"thermal cap hits\s*=\s*([0-9.eE+-]+)")

if new_steps is not None and new_pcorr is not None:
    print(
        f"  last-interval pressure correctors/step: "
        f"3.000 -> {new_pcorr/new_steps:.3f}"
    )
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

if cont_samples is not None and cont_samples > 0:
    print("  continuity diagnostics:")
    print(f"    samples        = {cont_samples:.0f}")
    print(f"    max |local|    = {cont_local:.6g}")
    print(f"    max |global|   = {cont_global:.6g}")
    print(f"    final cumul    = {cont_final:.6g}")
    print(f"    max |cumul|    = {cont_max_cum:.6g}")
else:
    print("  continuity diagnostics: unavailable; rerun ./SummarizeRun with current develop")

print()
print("Acceptance guide:")
print("  - W/D/L and source conservation should remain within the accepted envelope;")
print("  - pressure and total wall time should decrease materially;")
print("  - inspect continuity errors in the solver log before accepting;");
print("  - reject if velocity/free-surface evolution becomes less stable.")
