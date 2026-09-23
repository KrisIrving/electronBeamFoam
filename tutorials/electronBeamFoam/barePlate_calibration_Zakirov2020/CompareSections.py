#!/usr/bin/env python3
from pathlib import Path
import csv
import statistics

case = Path(__file__).resolve().parent
path = case / "postProcessing" / "meltPoolDiagnostics" / "fusionZoneSections.csv"

if not path.is_file():
    raise SystemExit(f"Missing {path}")

with path.open(newline="") as f:
    rows = list(csv.DictReader(f))
if not rows:
    raise SystemExit("fusionZoneSections.csv contains no rows")

final_time = max(float(r["time_s"]) for r in rows)
final = [
    r for r in rows
    if abs(float(r["time_s"]) - final_time) <= 1e-14
]

print("Station-wise fusion-zone cross sections")
print(f"  final time = {final_time:.12g} s")
print("  finite-slab diagnostic; positions are relative to referenceSurfacePoint")
print()
print("  station_mm    cells      W_um      D_um")
print("  ----------    -----    -------   -------")

valid = []
for r in sorted(final, key=lambda x: float(x["station_m"])):
    station = float(r["station_m"])*1e3
    cells = int(r["sectionCells"])
    width = float(r["width_m"])*1e6
    depth = float(r["depth_m"])*1e6
    print(f"  {station:+10.3f}    {cells:5d}    {width:7.3f}   {depth:7.3f}")
    if cells > 0:
        valid.append((station, width, depth))

if valid:
    widths = [r[1] for r in valid]
    depths = [r[2] for r in valid]
    print()
    print(
        f"  valid-station mean: W={statistics.fmean(widths):.3f} um  "
        f"D={statistics.fmean(depths):.3f} um"
    )
    print(
        f"  station spread:     W={max(widths)-min(widths):.3f} um  "
        f"D={max(depths)-min(depths):.3f} um"
    )

central = min(valid, key=lambda x: abs(x[0])) if valid else None
if central:
    print(
        f"  central station:     x={central[0]:+.3f} mm  "
        f"W={central[1]:.3f} um  D={central[2]:.3f} um"
    )

print()
print("Zakirov target stored for this case: W=525 um, D=51 um")
print(
    "Do not fit source parameters from a short-track section smoke test. "
    "Use the full 3 mm run and inspect station-to-station spread first."
)
