#!/usr/bin/env python3
"""Compare final cumulative fusion-zone W/D with Zakirov2020 Table 2."""
from pathlib import Path
import csv
import re
import sys

case = Path(__file__).resolve().parent
repo = case.parents[2]
fusion = case / "postProcessing" / "meltPoolDiagnostics" / "fusionZone.csv"
track_log = case / "log.generateTrack"
targets = repo / "validation" / "Zakirov2020_Ti64_beadOnPlate.csv"

if not fusion.is_file():
    raise SystemExit(f"Missing {fusion}")
if not track_log.is_file():
    raise SystemExit(f"Missing {track_log}")

text = track_log.read_text()
def grab(label):
    m = re.search(rf"^\s*{re.escape(label)}\s*=\s*([-+0-9.eE]+)", text, re.M)
    if not m:
        raise SystemExit(f"Could not parse {label} from log.generateTrack")
    return float(m.group(1))

preheat = grab("preheat")
power = grab("power")
speed_ms = grab("scan speed")
speed_mm_s = speed_ms*1000.0
track_length = grab("track length")

target = None
with targets.open(newline="") as f:
    for row in csv.DictReader(f):
        if (
            abs(float(row["preheat_K"]) - preheat) < 0.5
            and abs(float(row["power_W"]) - power) < 0.5
            and abs(float(row["scan_speed_mm_s"]) - speed_mm_s) < 0.5
        ):
            target = row
            break

if target is None:
    print("No exact Zakirov2020 experimental target for this override set.")
    sys.exit(0)

with fusion.open(newline="") as f:
    rows = list(csv.DictReader(f))
if not rows:
    raise SystemExit("fusionZone.csv has no data rows")

last = rows[-1]
w_sim = float(last["width_m"])*1e6
d_sim = float(last["depth_m"])*1e6
w_exp = float(target["experiment_width_um"])
d_exp = float(target["experiment_depth_um"])

w_err = 100.0*(w_sim - w_exp)/w_exp
d_err = 100.0*(d_sim - d_exp)/d_exp

print("Zakirov2020 final fusion-zone comparison")
if abs(track_length - 3.0e-3) > 1.0e-6:
    print(
        f"  note: track length is {track_length*1e3:.3f} mm; "
        "the reference numerical track length is 3.000 mm"
    )
    print("  SHORT-TRACK DIAGNOSTIC ONLY: do not use these errors for calibration.")
print(f"  Wsim = {w_sim:.3f} um    Wexp = {w_exp:.3f} um    error = {w_err:+.2f}%")
print(f"  Dsim = {d_sim:.3f} um    Dexp = {d_exp:.3f} um    error = {d_err:+.2f}%")
