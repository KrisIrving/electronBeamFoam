#!/usr/bin/env python3
"""Generate a parameterised moving single-track EPBF source history.

Environment overrides:
  BEAM_POWER      [W]   default 156
  SCAN_SPEED      [m/s] default 0.5
  X_START         [m]   default -0.00035
  X_END           [m]   default  0.00035
  BEAM_Y          [m]   default  0.00005
  BEAM_Z          [m]   default  0.00025
  COOL_TIME       [s]   default 0
  WRITE_INTERVAL  [s]   default 1e-4

The script also updates system/controlDict endTime/writeInterval. The runner
backs up and restores all tracked files, so parameter sweeps do not leave the
Git worktree dirty after a normal or trapped exit.
"""
from pathlib import Path
import math
import os
import re
import sys

root = Path(__file__).resolve().parent
constant = root / "constant"
control_dict = root / "system" / "controlDict"
beam_props = constant / "ElectronBeamProperties"

def env_float(name, default):
    raw = os.environ.get(name, str(default))
    try:
        value = float(raw)
    except ValueError:
        raise SystemExit(f"{name} must be numeric, got {raw!r}")
    if not math.isfinite(value):
        raise SystemExit(f"{name} must be finite")
    return value

power = env_float("BEAM_POWER", 156.0)
speed = env_float("SCAN_SPEED", 0.5)
x_start = env_float("X_START", -0.35e-3)
x_end = env_float("X_END", 0.35e-3)
beam_y = env_float("BEAM_Y", 0.05e-3)
beam_z = env_float("BEAM_Z", 0.25e-3)
cool_time = env_float("COOL_TIME", 0.0)
write_interval = env_float("WRITE_INTERVAL", 1.0e-4)

if power < 0.0:
    raise SystemExit("BEAM_POWER must be >= 0")
if speed <= 0.0:
    raise SystemExit("SCAN_SPEED must be > 0")
if x_start == x_end:
    raise SystemExit("X_START and X_END must differ")
if cool_time < 0.0:
    raise SystemExit("COOL_TIME must be >= 0")
if write_interval <= 0.0:
    raise SystemExit("WRITE_INTERVAL must be > 0")

props_text = beam_props.read_text()
match = re.search(r"^\s*beamRadius\s+([0-9.eE+-]+)\s*;", props_text, re.M)
if not match:
    raise SystemExit("Could not read beamRadius from ElectronBeamProperties")
beam_radius = float(match.group(1))

# The blockMesh x-domain is [-0.5, 0.5] mm. Keep the default 2*rb sampled
# multi-ray footprint inside the domain at both ends of the scan.
x_domain_min = -0.5e-3
x_domain_max = 0.5e-3
footprint_margin = 2.0*beam_radius

if min(x_start, x_end) - footprint_margin < x_domain_min:
    raise SystemExit(
        "Scan starts too close to the left boundary for the 2*beamRadius "
        "multi-ray footprint"
    )
if max(x_start, x_end) + footprint_margin > x_domain_max:
    raise SystemExit(
        "Scan ends too close to the right boundary for the 2*beamRadius "
        "multi-ray footprint"
    )

scan_length = abs(x_end - x_start)
scan_time = scan_length/speed
end_time = scan_time + cool_time

position_points = [
    (0.0, (x_start, beam_y, beam_z)),
    (scan_time, (x_end, beam_y, beam_z)),
]
if cool_time > 0.0:
    position_points.append((end_time, (x_end, beam_y, beam_z)))

power_points = [
    (0.0, power),
    (scan_time, power),
]
if cool_time > 0.0:
    eps = max(1e-12, 1e-9*scan_time)
    power_points.extend([
        (scan_time + eps, 0.0),
        (end_time, 0.0),
    ])

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

write_table(constant / "timeVsBeamPosition", position_points, vector=True)
write_table(constant / "timeVsBeamPower", power_points, vector=False)

control_text = control_dict.read_text()
control_text, n_end = re.subn(
    r"(^\s*endTime\s+)[^;]+;",
    rf"\g<1>{end_time:.12g};",
    control_text,
    count=1,
    flags=re.M,
)
control_text, n_write = re.subn(
    r"(^\s*writeInterval\s+)[^;]+;",
    rf"\g<1>{write_interval:.12g};",
    control_text,
    count=1,
    flags=re.M,
)
if n_end != 1 or n_write != 1:
    raise SystemExit("Could not update endTime/writeInterval in controlDict")
control_dict.write_text(control_text)

line_energy = power/speed if speed > 0 else float("inf")

print("Moving single-track EPBF case")
print(f"  power          = {power:.9g} W")
print(f"  scan speed     = {speed:.9g} m/s")
print(f"  x start/end    = {x_start:.9g} / {x_end:.9g} m")
print(f"  scan length    = {scan_length:.9g} m")
print(f"  scan time      = {scan_time:.9g} s")
print(f"  cool time      = {cool_time:.9g} s")
print(f"  end time       = {end_time:.9g} s")
print(f"  line energy    = {line_energy:.9g} J/m")
print(f"  write interval = {write_interval:.9g} s")
