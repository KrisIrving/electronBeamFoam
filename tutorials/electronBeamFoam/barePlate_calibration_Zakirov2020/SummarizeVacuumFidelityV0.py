#!/usr/bin/env python3
"""Summarize the diagnostic-only evaporation-mass V0 gate.

This script does not modify the case. It compares the evaporation diagnostic
with the cumulative fusion-zone diagnostic at their latest common write time.
"""

from pathlib import Path
import csv
import math
import re

case = Path(__file__).resolve().parent
evap_path = case / "postProcessing" / "evaporationDiagnostics" / "evaporation.csv"
fusion_path = case / "postProcessing" / "meltPoolDiagnostics" / "fusionZone.csv"
track_log = case / "log.generateTrack"

for p in (evap_path, fusion_path):
    if not p.is_file():
        raise SystemExit(f"Missing {p}")

def read_rows(path):
    with path.open(newline="") as f:
        return list(csv.DictReader(f))

evap = read_rows(evap_path)
fusion = read_rows(fusion_path)
if not evap or not fusion:
    raise SystemExit("Evaporation/fusion diagnostics contain no data rows")

def nearest(rows, target):
    return min(rows, key=lambda r: abs(float(r["time_s"]) - target))

evap_latest = max(evap, key=lambda r: float(r["time_s"]))
fusion_latest = max(fusion, key=lambda r: float(r["time_s"]))

te = float(evap_latest["time_s"])
tf = float(fusion_latest["time_s"])
common_t = min(te, tf)
erow = nearest(evap, common_t)
frow = nearest(fusion, common_t)

te_common = float(erow["time_s"])
tf_common = float(frow["time_s"])
if abs(te_common - tf_common) > 5e-9:
    raise SystemExit(
        "No sufficiently close common evaporation/fusion write time: "
        f"{te_common:.12g} vs {tf_common:.12g}"
    )

cum_mass = float(erow["cumulativeMass_kg"])
evap_volume = float(erow["equivalentMetalVolume_m3"])
evap_power = float(erow["evaporationPower_W"])
max_flux = float(erow["maxMassFlux_kg_per_m2_s"])
max_recession = float(erow["maxRecession_m_per_s"])
interface_area = float(erow["geometricInterfaceArea_m2"])
fusion_volume = float(frow["volume_m3"])

ratio = 100.0*evap_volume/fusion_volume if fusion_volume > 0 else float("nan")

power = None
absorptivity = None
target_end = None
if track_log.is_file():
    txt = track_log.read_text(errors="replace")
    patterns = {
        "power": r"^\s*power\s*=\s*([0-9.eE+-]+)\s*W\s*$",
        "absorptivity": r"^\s*absorptivity\s*=\s*([0-9.eE+-]+)\s*$",
        "end": r"^\s*end time\s*=\s*([0-9.eE+-]+)\s*s\s*$",
        "scan": r"^\s*scan time\s*=\s*([0-9.eE+-]+)\s*s\s*$",
        "cool": r"^\s*cool time\s*=\s*([0-9.eE+-]+)\s*s\s*$",
    }
    vals = {}
    for key, pat in patterns.items():
        m = re.search(pat, txt, flags=re.M)
        vals[key] = float(m.group(1)) if m else None
    power = vals["power"]
    absorptivity = vals["absorptivity"]
    target_end = vals["end"]
    if target_end is None and vals["scan"] is not None:
        target_end = vals["scan"] + (vals["cool"] or 0.0)

absorbed = (
    power*absorptivity
    if power is not None and absorptivity is not None
    else None
)
power_fraction = (
    100.0*evap_power/absorbed
    if absorbed is not None and absorbed > 0
    else None
)

complete = (
    target_end is not None
    and te_common >= target_end - max(1e-12, 1e-7*abs(target_end))
)

print("Vacuum Fidelity V0: implied evaporation mass loss")
print(f"  diagnostic write time     = {te_common:.12g} s")
if target_end is not None:
    print(f"  generated case end        = {target_end:.12g} s")
    print(
        f"  diagnostic completion     = "
        f"{100.0*min(max(te_common/target_end,0.0),1.0):.2f}%"
    )
print(f"  cumulative evap mass      = {cum_mass:.6e} kg")
print(f"  equivalent evap volume    = {evap_volume:.6e} m3")
print(f"  fusion-zone volume        = {fusion_volume:.6e} m3")
print(f"  evap/fusion volume        = {ratio:.6f}%")
print(f"  evaporation power         = {evap_power:.6f} W")
if absorbed is not None:
    print(f"  absorbed beam power       = {absorbed:.6f} W")
    print(f"  evap/absorbed power       = {power_fraction:.6f}%")
print(f"  max evaporation mass flux = {max_flux:.6e} kg/m2/s")
print(f"  max recession speed       = {max_recession:.6e} m/s")
print(f"  geometric interface area  = {interface_area:.6e} m2")
print(
    "  status                    = "
    + ("FINAL WRITE AVAILABLE" if complete else "INCOMPLETE / LIVE")
)
print()
print("Interpretation:")
print("  This is a diagnostic conversion of the evaporation latent-heat sink.")
print("  It does not yet remove metal mass from the VOF field.")
print("  A final physics decision must use the completed case, not a live write.")
