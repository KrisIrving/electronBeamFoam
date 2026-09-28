#!/usr/bin/env python3
from pathlib import Path
import csv
import json
import re

case = Path(__file__).resolve().parent
repo = case.parents[2]
baseline_path = repo / "validation" / "Zakirov2020_decomp_simpleXZ_1mm.json"
fz_path = case / "postProcessing" / "meltPoolDiagnostics" / "fusionZone.csv"
timing_path = case / "redistribution-1mm-timing.txt"
prebalance_path = case / "redistribution-1mm-prebalance.txt"
balance_path = case / "redistribution-1mm-balance.txt"
stage2_path = case / "log.redistribution1mm.stage2"

for p in (
    baseline_path,
    fz_path,
    timing_path,
    prebalance_path,
    balance_path,
    stage2_path,
):
    if not p.is_file():
        raise SystemExit(f"Missing {p}")

baseline = json.loads(baseline_path.read_text())
with fz_path.open(newline="") as f:
    rows = list(csv.DictReader(f))
if not rows:
    raise SystemExit("fusionZone.csv has no data rows")

target_t = float(baseline["probe"]["final_write_time_s"])
row = min(rows, key=lambda r: abs(float(r["time_s"]) - target_t))
candidate_t = float(row["time_s"])
if abs(candidate_t - target_t) > 5e-8:
    raise SystemExit(
        f"No candidate fusion-zone row sufficiently close to baseline write "
        f"time {target_t:.12g}; nearest is {candidate_t:.12g}"
    )

def parse_kv(path):
    out = {}
    for line in path.read_text(errors="replace").splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            try:
                out[k.strip()] = float(v.strip())
            except ValueError:
                pass
    return out

def pct(new, old):
    return 100.0 * (new - old) / (old if abs(old) > 1e-30 else 1.0)

timing = parse_kv(timing_path)
base = baseline["probe"]["final_global_fusion_zone"]
cand = {
    "length_m": float(row["length_m"]),
    "width_m": float(row["width_m"]),
    "depth_m": float(row["depth_m"]),
    "volume_m3": float(row["volume_m3"]),
}

baseline_wall = float(baseline["probe"]["clock_time_s"])
segmented_wall = timing["segmented_solver_wall_s"]
workflow_wall = timing["workflow_wall_s"]

print("1 mm checkpoint redistribution performance gate")
print("  reference = uninterrupted simpleXZ 8x1x6")
print("  candidate = simpleXZ -> t=2e-4 s -> Scotch -> resume")
print(f"  comparison write time = {candidate_t:.12g} s")
print()

deltas = {}
for label, key, scale, unit in (
    ("W", "width_m", 1e6, "um"),
    ("D", "depth_m", 1e6, "um"),
    ("L", "length_m", 1e6, "um"),
    ("V", "volume_m3", 1e9, "mm^3"),
):
    old = float(base[key])
    new = cand[key]
    d = pct(new, old)
    deltas[label] = d
    print(
        f"  {label}: {old*scale:.6g} -> {new*scale:.6g} {unit}  "
        f"delta={d:+.3f}%"
    )

print()
print("performance")
print(f"  uninterrupted baseline clock = {baseline_wall:.0f} s")
print(f"  segmented solver wall        = {segmented_wall:.0f} s")
print(f"  full redistribution workflow = {workflow_wall:.0f} s")
print(f"  solver-only speedup          = {baseline_wall/segmented_wall:.3f}x")
print(f"  workflow speedup             = {baseline_wall/workflow_wall:.3f}x")

print()
print(prebalance_path.read_text().strip())
print()
print(balance_path.read_text().strip())

stage2 = stage2_path.read_text(errors="replace")
imb = re.findall(r"cell imbalance max/mean\s*=\s*([0-9.eE+-]+)", stage2)
if imb:
    print(f"  final post-restart max/mean imbalance = {float(imb[-1]):.6f}")
refined = len(re.findall(r"Refined from", stage2))
unrefined = len(re.findall(r"Unrefined from", stage2))
print(f"  post-redistribution AMR events: refined={refined}, unrefined={unrefined}")

fidelity_ok = (
    abs(deltas["W"]) <= 0.5
    and abs(deltas["D"]) <= 0.5
    and abs(deltas["L"]) <= 0.5
    and abs(deltas["V"]) <= 1.0
)
workflow_speedup = baseline_wall / workflow_wall

print()
print("gate interpretation")
print(
    "  fidelity = "
    + ("PASS" if fidelity_ok else "REVIEW")
    + "  (W/D/L <=0.5%, V <=1.0%)"
)
if workflow_speedup > 1.05:
    perf = "PASS (>5% workflow speedup)"
elif workflow_speedup > 1.0:
    perf = "MARGINAL (0-5% workflow speedup)"
else:
    perf = "REJECT (no workflow speedup)"
print(f"  performance = {perf}")
print()
print("Decision rule:")
print("  - fidelity must pass before any campaign use;")
print("  - full workflow wall time, not cell balance alone, decides performance;")
print("  - if rejected, retain simpleXZ/static baseline and close this")
print("    redistribution optimisation branch before returning to physics.")
