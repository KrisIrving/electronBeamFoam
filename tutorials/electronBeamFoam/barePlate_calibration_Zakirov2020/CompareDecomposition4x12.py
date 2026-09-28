#!/usr/bin/env python3
from pathlib import Path
import json
import re

case = Path(__file__).resolve().parent
repo = case.parents[2]
ref_path = repo / "validation" / "Zakirov2020_decomp_simpleXZ_0p5mm.json"
summary_path = case / "run-summary.txt"

for p in (ref_path, summary_path):
    if not p.is_file():
        raise SystemExit(f"Missing {p}")

ref = json.loads(ref_path.read_text())
text = summary_path.read_text(errors="replace")

def last(pattern, cast=float):
    vals = re.findall(pattern, text)
    return cast(vals[-1]) if vals else None

def fz(label):
    blocks = re.findall(r"Fusion-zone diagnostics\n(?:    .*\n)+", text)
    if not blocks:
        return None
    vals = re.findall(rf"    {label}\s*=\s*([0-9.eE+-]+)", blocks[-1])
    return float(vals[-1]) if vals else None

new = {
    "width_m": fz("width"),
    "depth_m": fz("depth"),
    "length_m": fz("length"),
    "volume_m3": fz("volume"),
}
old = ref["probe"]["fusionZone"]

print("Calibration decomposition geometry A/B")
print("  reference = simpleXZ 8x1x6, accepted 0.5 mm gate")
print("  probe     = simpleXZ 4x1x12, 48 ranks")
print()

for label,key,scale,unit in (
    ("W","width_m",1e6,"um"),
    ("D","depth_m",1e6,"um"),
    ("L","length_m",1e6,"um"),
    ("V","volume_m3",1e9,"mm^3"),
):
    v=new[key]
    if v is None:
        continue
    a=float(old[key])
    rel=100.0*(v-a)/(a if abs(a)>1e-30 else 1.0)
    print(f"  {label}: {a*scale:.6g} -> {v*scale:.6g} {unit}   delta={rel:+.3f}%")

local = re.findall(r"local cells min/max\s*=\s*([0-9]+)\s*/\s*([0-9]+)", text)
imb = last(r"cell imbalance max/mean\s*=\s*([0-9.eE+-]+)")
steps = last(r"steps\s*=\s*([0-9.eE+-]+)")
wall = last(r"wall total\s*=\s*([0-9.eE+-]+) s")
pressure = last(r"pressure\s*=\s*([0-9.eE+-]+) s")
thermal = last(r"thermal/phase\s*=\s*([0-9.eE+-]+) s")
clock = last(r"ClockTime = ([0-9.eE+-]+) s")

rfi=ref["probe"]["final_interval"]

print()
print(f"  reference imbalance: {rfi['cell_imbalance_max_over_mean']:.3f}")
if imb is not None:
    print(f"  probe imbalance:     {imb:.3f}")
    print(f"  imbalance ratio:     {imb/rfi['cell_imbalance_max_over_mean']:.3f}x")
if local:
    lo,hi=map(int,local[-1])
    print(f"  probe local cells min/max: {lo} / {hi}   max/min={hi/lo:.3f}x")

if steps is not None and wall is not None:
    print(f"  probe final-interval wall/step: {wall/steps:.6f} s/step")
    print(f"  reference wall/step:            {rfi['wall_total_s']/rfi['steps']:.6f} s/step")
if wall is not None:
    print(f"  final-interval total wall: {rfi['wall_total_s']:.3f} -> {wall:.3f} s")
if pressure is not None:
    print(f"  pressure wall: {rfi['pressure_s']:.3f} -> {pressure:.3f} s")
if thermal is not None:
    print(f"  thermal wall: {rfi['thermal_phase_s']:.3f} -> {thermal:.3f} s")
if clock is not None:
    print(f"  ClockTime: {ref['probe']['clock_time_s']:.1f} -> {clock:.1f} s")

print()
print("Advance to 1 mm only if:")
print("  - W/D/L remain inside the accepted numerical envelope;")
print("  - imbalance is lower than 8x1x6;")
print("  - actual and per-step wall cost remain competitive;")
print("  - no new stability warning appears.")
