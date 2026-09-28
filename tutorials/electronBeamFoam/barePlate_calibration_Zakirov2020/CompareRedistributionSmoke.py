#!/usr/bin/env python3
from pathlib import Path
import csv
import json
import re

case = Path(__file__).resolve().parent
repo = case.parents[2]
baseline_path = repo / "validation" / "Zakirov2020_decomp_simpleXZ_0p5mm.json"
summary_path = case / "run-summary.txt"
balance_path = case / "redistribution-balance.txt"
timing_path = case / "redistribution-smoke-timing.txt"
fz_path = case / "postProcessing" / "meltPoolDiagnostics" / "fusionZone.csv"
stage2_path = case / "log.redistribution.stage2"

for p in (baseline_path, summary_path, balance_path, timing_path, fz_path, stage2_path):
    if not p.is_file():
        raise SystemExit(f"Missing {p}")

baseline = json.loads(baseline_path.read_text())
summary = summary_path.read_text(errors="replace")
balance = balance_path.read_text(errors="replace")
timing = timing_path.read_text(errors="replace")
stage2 = stage2_path.read_text(errors="replace")

with fz_path.open(newline="") as f:
    rows = list(csv.DictReader(f))
if len(rows) < 2:
    raise SystemExit("Expected at least midpoint and final fusion-zone rows")

rows = sorted(rows, key=lambda r: float(r["time_s"]))
mid = rows[0]
final = rows[-1]

def pct(new, old):
    return 100.0*(new-old)/(old if abs(old)>1e-30 else 1.0)

b = baseline["probe"]["fusionZone"]
vals = {
    "length_m": float(final["length_m"]),
    "width_m": float(final["width_m"]),
    "depth_m": float(final["depth_m"]),
    "volume_m3": float(final["volume_m3"]),
}

print("Checkpoint redistribution smoke test")
print("  initial decomposition = simpleXZ 8x1x6")
print("  checkpoint target     = Scotch on current refined mesh")
print("  total physical track  = 0.5 mm")
print()

for label,key,scale,unit in (
    ("W","width_m",1e6,"um"),
    ("D","depth_m",1e6,"um"),
    ("L","length_m",1e6,"um"),
    ("V","volume_m3",1e9,"mm^3"),
):
    old=float(b[key]); new=vals[key]
    print(
        f"  {label}: {old*scale:.6g} -> {new*scale:.6g} {unit}  "
        f"delta={pct(new,old):+.3f}%"
    )

print()
print("  cumulative fusion history:")
print(
    f"    first stored row t={float(mid['time_s']):.12g} s "
    f"L={float(mid['length_m'])*1e6:.3f} um "
    f"V={float(mid['volume_m3'])*1e9:.6f} mm^3"
)
print(
    f"    final row        t={float(final['time_s']):.12g} s "
    f"L={float(final['length_m'])*1e6:.3f} um "
    f"V={float(final['volume_m3'])*1e9:.6f} mm^3"
)

print()
print(balance.strip())
print()
print("timing (smoke workflow; NOT a campaign performance comparison)")
for line in timing.strip().splitlines():
    print(f"  {line}")

refined = len(re.findall(r"Refined from", stage2))
unrefined = len(re.findall(r"Unrefined from", stage2))
print()
print(f"  post-redistribution AMR events: refined={refined}, unrefined={unrefined}")

for name in ("preRedistribute", "postRedistribute"):
    p = case / f"checkpoint-files-{name}.txt"
    if p.is_file():
        print()
        print(p.read_text().strip())

print()
print("Acceptance gate:")
print("  - final W/D/L must reproduce the accepted 0.5 mm baseline;")
print("  - everMelted/peakTemperature and hexRef8 checkpoint files must survive;")
print("  - dynamic refine/unrefine must occur after restart;")
print("  - checkMesh must pass after redistribution;")
print("  - only after fidelity passes should redistribution be tested for 1 mm speed.")
