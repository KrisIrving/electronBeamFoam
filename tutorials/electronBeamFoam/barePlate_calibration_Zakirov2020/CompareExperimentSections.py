#!/usr/bin/env python3
"""Compare final central-track fusion sections with the Zakirov2020 target."""
from pathlib import Path
import csv
import re
import statistics
import sys

case = Path(__file__).resolve().parent
repo = case.parents[2]
sections = case / "postProcessing" / "meltPoolDiagnostics" / "fusionZoneSections.csv"
track_log = case / "log.generateTrack"
targets = repo / "validation" / "Zakirov2020_Ti64_beadOnPlate.csv"

for p in (sections, track_log, targets):
    if not p.is_file():
        raise SystemExit(f"Missing {p}")

text = track_log.read_text()

def grab(label):
    m = re.search(rf"^\s*{re.escape(label)}\s*=\s*([-+0-9.eE]+)", text, re.M)
    if not m:
        raise SystemExit(f"Could not parse {label} from log.generateTrack")
    return float(m.group(1))

preheat = grab("preheat")
power = grab("power")
speed_ms = grab("scan speed")
track_length = grab("track length")

target = None
with targets.open(newline="") as f:
    for row in csv.DictReader(f):
        if (
            abs(float(row["preheat_K"]) - preheat) < 0.5
            and abs(float(row["power_W"]) - power) < 0.5
            and abs(float(row["scan_speed_mm_s"]) - speed_ms*1000.0) < 0.5
        ):
            target = row
            break

if target is None:
    print("No exact Zakirov2020 experimental target for this override set.")
    sys.exit(0)

with sections.open(newline="") as f:
    rows = list(csv.DictReader(f))
if not rows:
    raise SystemExit("fusionZoneSections.csv has no rows")

final_time = max(float(r["time_s"]) for r in rows)
final = [
    r for r in rows
    if abs(float(r["time_s"]) - final_time) <= 1e-14
    and int(r["sectionCells"]) > 0
]

if not final:
    raise SystemExit("No non-zero fusion-zone sections at final time")

w_exp = float(target["experiment_width_um"])
d_exp = float(target["experiment_depth_um"])

data = []
for r in sorted(final, key=lambda x: float(x["station_m"])):
    x_mm = float(r["station_m"])*1e3
    w = float(r["width_m"])*1e6
    d = float(r["depth_m"])*1e6
    data.append((x_mm, w, d))

print("Zakirov2020 station-wise fusion-zone comparison")
print(f"  final time   = {final_time:.12g} s")
print(f"  track length = {track_length*1e3:.3f} mm")
if abs(track_length - 3.0e-3) > 1.0e-6:
    print("  SHORT-TRACK DIAGNOSTIC ONLY: do not use these errors for calibration.")
print(f"  experiment   = W {w_exp:.3f} um, D {d_exp:.3f} um")
print()
print("  station_mm      W_um    W_err_%      D_um    D_err_%")
print("  ----------    -------   -------    -------   -------")
for x_mm, w, d in data:
    print(
        f"  {x_mm:+10.3f}    {w:7.3f}   {100*(w-w_exp)/w_exp:+7.2f}    "
        f"{d:7.3f}   {100*(d-d_exp)/d_exp:+7.2f}"
    )

widths = [v[1] for v in data]
depths = [v[2] for v in data]
w_mean = statistics.fmean(widths)
d_mean = statistics.fmean(depths)
central = min(data, key=lambda v: abs(v[0]))

print()
print(
    f"  central-window mean: W={w_mean:.3f} um "
    f"({100*(w_mean-w_exp)/w_exp:+.2f}%), "
    f"D={d_mean:.3f} um ({100*(d_mean-d_exp)/d_exp:+.2f}%)"
)
print(
    f"  station spread:      dW={max(widths)-min(widths):.3f} um, "
    f"dD={max(depths)-min(depths):.3f} um"
)
print(
    f"  central x={central[0]:+.3f} mm: "
    f"W={central[1]:.3f} um, D={central[2]:.3f} um"
)
print()
if abs(track_length - 3.0e-3) <= 1.0e-6:
    print(
        "  PRIMARY QUANTITATIVE OBSERVABLE: use the central-window mean "
        "together with station spread; retain global fusionZone.csv as a "
        "secondary whole-track envelope."
    )
else:
    print(
        "  This short-track result validates the section diagnostic only. "
        "The first quantitative comparison requires the 3 mm reference run."
    )
