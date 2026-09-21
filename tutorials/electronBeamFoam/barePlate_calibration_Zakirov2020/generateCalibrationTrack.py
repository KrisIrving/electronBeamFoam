#!/usr/bin/env python3
"""Generate the Zakirov2020 Ti-6Al-4V bead-on-plate calibration run.

Defaults reproduce the first calibration target selected in validation/:
  preheat      296 K
  voltage      60 kV (metadata; not explicitly solved)
  current      15 mA (metadata)
  power        900 W
  scan speed   3.0 m/s = 3000 mm/s
  track length 3.0 mm
  D4sigma      500 um -> Gaussian 1/e^2 radius rb=250 um
  absorptivity 0.85
  penetration  18.9 um

Environment overrides:
  PREHEAT_K, BEAM_POWER, SCAN_SPEED, X_START, X_END, BEAM_Y, BEAM_Z,
  ABSORPTIVITY, BEAM_RADIUS, PENETRATION_DEPTH, MAX_PENETRATION_DEPTH,
  COOL_TIME, WRITE_INTERVAL.

Tracked dictionaries are restored by Allrun_parallel after the run.
"""
from pathlib import Path
import math
import os
import re

root = Path(__file__).resolve().parent
constant = root / "constant"
control_dict = root / "system" / "controlDict"
beam_props = constant / "ElectronBeamProperties"
transport_props = constant / "transportProperties"
initial_t = root / "initial" / "T"

def env_float(name, default):
    raw = os.environ.get(name, str(default))
    try:
        value = float(raw)
    except ValueError:
        raise SystemExit(f"{name} must be numeric, got {raw!r}")
    if not math.isfinite(value):
        raise SystemExit(f"{name} must be finite")
    return value

preheat = env_float("PREHEAT_K", 296.0)
power = env_float("BEAM_POWER", 900.0)
speed = env_float("SCAN_SPEED", 3.0)
x_start = env_float("X_START", -1.5e-3)
x_end = env_float("X_END", 1.5e-3)
beam_y = env_float("BEAM_Y", 0.05e-3)
beam_z = env_float("BEAM_Z", 0.75e-3)
absorptivity = env_float("ABSORPTIVITY", 0.85)
beam_radius = env_float("BEAM_RADIUS", 250e-6)
penetration = env_float("PENETRATION_DEPTH", 18.9e-6)
max_penetration = env_float("MAX_PENETRATION_DEPTH", 120e-6)
cool_time = env_float("COOL_TIME", 0.0)
write_requested = "WRITE_INTERVAL" in os.environ
write_interval = env_float("WRITE_INTERVAL", 1.0e-4)

for name, value in (
    ("PREHEAT_K", preheat),
    ("BEAM_POWER", power),
    ("SCAN_SPEED", speed),
    ("ABSORPTIVITY", absorptivity),
    ("BEAM_RADIUS", beam_radius),
    ("PENETRATION_DEPTH", penetration),
    ("MAX_PENETRATION_DEPTH", max_penetration),
    ("WRITE_INTERVAL", write_interval),
):
    if value <= 0:
        raise SystemExit(f"{name} must be > 0")

if not (0 < absorptivity <= 1):
    raise SystemExit("ABSORPTIVITY must be in (0,1]")
if x_start == x_end:
    raise SystemExit("X_START and X_END must differ")
if cool_time < 0:
    raise SystemExit("COOL_TIME must be >= 0")

# Calibration mesh dimensions, in metres.
x_min, x_max = -2.2e-3, 2.2e-3
z_min, z_max = 0.0, 1.5e-3
surface_y = 0.2e-3
beamlet_radius_factor = 2.0
footprint = beamlet_radius_factor*beam_radius

if min(x_start, x_end) - footprint <= x_min:
    raise SystemExit("Beam footprint reaches the left x boundary")
if max(x_start, x_end) + footprint >= x_max:
    raise SystemExit("Beam footprint reaches the right x boundary")
if beam_z - footprint <= z_min or beam_z + footprint >= z_max:
    raise SystemExit("Beam footprint reaches a transverse z boundary")
if not (0.0 < beam_y < surface_y):
    raise SystemExit("BEAM_Y must lie in the vacuum headspace above the initial surface")

scan_length = abs(x_end - x_start)
scan_time = scan_length/speed
end_time = scan_time + cool_time

if not write_requested:
    write_interval = min(write_interval, 0.5*end_time)
if write_interval > end_time:
    raise SystemExit("WRITE_INTERVAL exceeds endTime")

def replace_scalar(text, key, value):
    pattern = rf"(^\s*{re.escape(key)}\s+)[^;]+;"
    text, n = re.subn(pattern, rf"\g<1>{value:.12g};", text, count=1, flags=re.M)
    if n != 1:
        raise SystemExit(f"Could not update {key}")
    return text

beam_text = beam_props.read_text()
for key, value in (
    ("absorptivity", absorptivity),
    ("beamRadius", beam_radius),
    ("penetrationDepth", penetration),
    ("maxPenetrationDepth", max_penetration),
    ("firstHitSearchRadius", 0.5*beam_radius),
):
    beam_text = replace_scalar(beam_text, key, value)
beam_props.write_text(beam_text)

transport_text = transport_props.read_text()
transport_text, n = re.subn(
    r"(^\s*TRef\s+TRef\s+\[0 0 0 1 0 0 0\]\s+)[^;]+;",
    rf"\g<1>{preheat:.12g};",
    transport_text, count=1, flags=re.M
)
if n != 1:
    raise SystemExit("Could not update TRef")
transport_props.write_text(transport_text)

t_text = initial_t.read_text()
t_text, n = re.subn(
    r"(^\s*internalField\s+uniform\s+)[^;]+;",
    rf"\g<1>{preheat:.12g};",
    t_text, count=1, flags=re.M
)
if n != 1:
    raise SystemExit("Could not update initial T")
initial_t.write_text(t_text)

position_rows = [
    (0.0, (x_start, beam_y, beam_z)),
    (scan_time, (x_end, beam_y, beam_z)),
]
if cool_time > 0:
    position_rows.append((end_time, (x_end, beam_y, beam_z)))

power_rows = [(0.0, power), (scan_time, power)]
if cool_time > 0:
    eps = max(1e-12, 1e-9*scan_time)
    power_rows += [(scan_time + eps, 0.0), (end_time, 0.0)]

def write_table(path, rows, vector=False):
    with path.open("w") as f:
        f.write("(\n")
        for t, value in rows:
            if vector:
                x, y, z = value
                f.write(f"    ({t:.12g} ({x:.12g} {y:.12g} {z:.12g}))\n")
            else:
                f.write(f"    ({t:.12g} {value:.12g})\n")
        f.write(")\n")

write_table(constant / "timeVsBeamPosition", position_rows, True)
write_table(constant / "timeVsBeamPower", power_rows, False)

control_text = control_dict.read_text()
control_text = replace_scalar(control_text, "endTime", end_time)
control_text = replace_scalar(control_text, "writeInterval", write_interval)
control_dict.write_text(control_text)

line_energy = power/speed
print("Zakirov2020 Ti-6Al-4V bead-on-plate calibration")
print(f"  preheat        = {preheat:.9g} K")
print(f"  power          = {power:.9g} W")
print(f"  scan speed     = {speed:.9g} m/s")
print(f"  x start/end    = {x_start:.9g} / {x_end:.9g} m")
print(f"  track length   = {scan_length:.9g} m")
print(f"  scan time      = {scan_time:.9g} s")
print(f"  line energy    = {line_energy:.9g} J/m")
print(f"  absorptivity   = {absorptivity:.9g}")
print(f"  beam radius    = {beam_radius:.9g} m")
print(f"  penetration    = {penetration:.9g} m")
print(f"  beam seed      = ({x_start:.9g}, {beam_y:.9g}, {beam_z:.9g}) m")
print(f"  write interval = {write_interval:.9g} s")
print("  experiment W   = 525 um")
print("  experiment D   = 51 um")
