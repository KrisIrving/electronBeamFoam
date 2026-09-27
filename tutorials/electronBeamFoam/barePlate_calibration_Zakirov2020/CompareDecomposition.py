#!/usr/bin/env python3
from pathlib import Path
import json
import re

case = Path(__file__).resolve().parent
repo = case.parents[2]
baseline_path = repo / "validation" / "Zakirov2020_preflight_accepted_baseline.json"
summary_path = case / "run-summary.txt"

for p in (baseline_path, summary_path):
    if not p.is_file():
        raise SystemExit(f"Missing {p}")

baseline = json.loads(baseline_path.read_text())
text = summary_path.read_text(errors="replace")

def last(pattern, cast=float):
    vals = re.findall(pattern, text)
    return cast(vals[-1]) if vals else None

def fz(label):
    block = re.findall(
        r"Fusion-zone diagnostics\n"
        r"(?:    .*\n)+",
        text
    )
    if not block:
        return None
    vals = re.findall(rf"    {label}\s*=\s*([0-9.eE+-]+)", block[-1])
    return float(vals[-1]) if vals else None

new_w = fz("width")
new_d = fz("depth")
new_l = fz("length")
new_v = fz("volume")

print("Calibration decomposition A/B")
print("  baseline = scotch, accepted pCorr2 0.5 mm run")
print("  probe    = simpleXZ n=(8 1 6), 48 ranks")
print()

for label, key, val, scale, unit in (
    ("W", "width_m", new_w, 1e6, "um"),
    ("D", "depth_m", new_d, 1e6, "um"),
    ("L", "length_m", new_l, 1e6, "um"),
    ("V", "volume_m3", new_v, 1e9, "mm^3"),
):
    if val is None:
        continue
    old = float(baseline["fusionZone"][key])
    rel = 100.0*(val-old)/(old if abs(old) > 1e-30 else 1.0)
    print(f"  {label}: {old*scale:.6g} -> {val*scale:.6g} {unit}   delta={rel:+.3f}%")

global_cells = last(r"global cells\s*=\s*([0-9.eE+-]+)")
local = re.findall(r"local cells min/max\s*=\s*([0-9]+)\s*/\s*([0-9]+)", text)
imb = last(r"cell imbalance max/mean\s*=\s*([0-9.eE+-]+)")
heavy = last(r"heaviest rank\s*=\s*([0-9]+)", int)
wall = last(r"wall total\s*=\s*([0-9.eE+-]+) s")
pressure = last(r"pressure\s*=\s*([0-9.eE+-]+) s")
thermal = last(r"thermal/phase\s*=\s*([0-9.eE+-]+) s")
clock = last(r"ClockTime = ([0-9.eE+-]+) s")

print()
if global_cells is not None:
    print(f"  final global cells: {global_cells:.0f}")
if local:
    lo, hi = map(int, local[-1])
    print(f"  local cells min/max: {lo} / {hi}   max/min={hi/lo:.3f}x")
if imb is not None:
    print(f"  cell imbalance max/mean: {imb:.3f}")
if heavy is not None:
    print(f"  heaviest rank: {heavy}")

old = baseline["final_interval"]
if pressure is not None:
    print(
        f"  last-interval pressure wall: {old['pressure_s']:.3f} -> "
        f"{pressure:.3f} s   speedup={old['pressure_s']/pressure:.3f}x"
    )
if thermal is not None:
    print(
        f"  last-interval thermal wall: {old['thermal_phase_s']:.3f} -> "
        f"{thermal:.3f} s   speedup={old['thermal_phase_s']/thermal:.3f}x"
    )
if wall is not None:
    print(
        f"  last-interval total wall: {old['wall_total_s']:.3f} -> "
        f"{wall:.3f} s   speedup={old['wall_total_s']/wall:.3f}x"
    )
if clock is not None:
    print(
        f"  ClockTime: {baseline['clock_time_s']:.1f} -> {clock:.1f} s   "
        f"speedup={baseline['clock_time_s']/clock:.3f}x"
    )

print()
print("Decision guide:")
print("  - fusion-zone W/D/L must remain within the mesh-resolution envelope;")
print("  - prefer a large reduction in max/mean cell imbalance;")
print("  - require lower wall time, not just prettier cell counts;")
print("  - if simpleXZ is promising, validate it on a longer 1 mm track before")
print("    replacing scotch for the multi-condition calibration campaign.")
