#!/usr/bin/env python3
from pathlib import Path
import argparse
import csv
import json
import re

parser = argparse.ArgumentParser()
parser.add_argument("--label", required=True)
parser.add_argument("--scale", type=float, required=True)
parser.add_argument("--interpolation", default="linear")
args = parser.parse_args()

case = Path(__file__).resolve().parent

def rows(path):
    p = case / path
    if not p.is_file():
        raise SystemExit(f"Missing {p}")
    with p.open(newline="") as f:
        data = list(csv.DictReader(f))
    if not data:
        raise SystemExit(f"No rows in {p}")
    return data

def latest(path):
    return max(rows(path), key=lambda r: float(r["time_s"]))

fusion = latest("postProcessing/meltPoolDiagnostics/fusionZone.csv")
melt = latest("postProcessing/meltPoolDiagnostics/meltPool.csv")
evap = latest("postProcessing/evaporationDiagnostics/evaporation.csv")

times = [float(fusion["time_s"]), float(melt["time_s"]), float(evap["time_s"])]
if max(times) - min(times) > 5e-9:
    raise SystemExit(f"Final diagnostic times disagree: {times}")

track_text = (case / "log.generateTrack").read_text(errors="replace")
solver_text = (case / "log.electronBeamFoam").read_text(errors="replace")

def grab(pattern, text, default=None):
    m = re.search(pattern, text, flags=re.M)
    return float(m.group(1)) if m else default

end_time = grab(r"^\s*end time\s*=\s*([0-9.eE+-]+)\s*s\s*$", track_text)
if end_time is None:
    scan = grab(r"^\s*scan time\s*=\s*([0-9.eE+-]+)\s*s\s*$", track_text)
    cool = grab(r"^\s*cool time\s*=\s*([0-9.eE+-]+)\s*s\s*$", track_text, 0.0)
    end_time = scan + cool if scan is not None else None

if end_time is None:
    raise SystemExit("Could not determine generated case end")

final_t = max(times)
if final_t < end_time - max(1e-12, 1e-7*abs(end_time)):
    raise SystemExit(
        f"Diagnostics are incomplete: final write {final_t:.12g}, "
        f"case end {end_time:.12g}"
    )

clock_matches = re.findall(
    r"ExecutionTime = [0-9.eE+-]+ s\s+ClockTime = ([0-9.eE+-]+) s",
    solver_text,
)
clock_time = float(clock_matches[-1]) if clock_matches else None

power_errors = re.findall(
    r"power error\s*=\s*([0-9.eE+-]+)\s*W",
    solver_text,
)
power_error = float(power_errors[-1]) if power_errors else None

imbalance = re.findall(
    r"cell imbalance max/mean\s*=\s*([0-9.eE+-]+)",
    solver_text,
)
final_imbalance = float(imbalance[-1]) if imbalance else None

summary = {
    "label": args.label,
    "void_kappa_scale": args.scale,
    "thermal_kappa_interpolation": args.interpolation,
    "final_time_s": final_t,
    "clock_time_s": clock_time,
    "power_error_W": power_error,
    "final_cell_imbalance_max_over_mean": final_imbalance,
    "fusion_zone": {
        "length_m": float(fusion["length_m"]),
        "width_m": float(fusion["width_m"]),
        "depth_m": float(fusion["depth_m"]),
        "volume_m3": float(fusion["volume_m3"]),
        "peakT_K": float(fusion["peakTmax_K"]),
    },
    "melt_pool": {
        "length_m": float(melt["length_m"]),
        "width_m": float(melt["width_m"]),
        "depth_m": float(melt["depth_m"]),
        "volume_m3": float(melt["volume_m3"]),
        "Tmax_K": float(melt["Tmax_K"]),
        "Umax_m_per_s": float(melt["Umax_m_per_s"]),
        "recoilMax_Pa": float(melt["recoilMax_Pa"]),
    },
    "evaporation": {
        "massRate_kg_per_s": float(evap["massRate_kg_per_s"]),
        "cumulativeMass_kg": float(evap["cumulativeMass_kg"]),
        "equivalentMetalVolume_m3": float(evap["equivalentMetalVolume_m3"]),
        "evaporationPower_W": float(evap["evaporationPower_W"]),
        "maxMassFlux_kg_per_m2_s": float(evap["maxMassFlux_kg_per_m2_s"]),
        "maxRecession_m_per_s": float(evap["maxRecession_m_per_s"]),
        "geometricInterfaceArea_m2": float(evap["geometricInterfaceArea_m2"]),
    },
}

print(json.dumps(summary, indent=2))
