#!/usr/bin/env python3
from pathlib import Path

n_tracks = 5
x_min = -0.15e-3
x_max =  0.15e-3
z0 = 0.10e-3
hatch = 0.06e-3
beam_y = 0.05e-3
scan_speed = 0.50
beam_power = 156.0
jump_time = 20e-6
t0 = 0.0

root = Path(__file__).resolve().parent
pos_file = root / "constant" / "timeVsBeamPosition"
power_file = root / "constant" / "timeVsBeamPower"

eps = 1e-12
points = []
powers = []
t = t0
track_time = abs(x_max - x_min)/scan_speed

points.append((t, (x_min, beam_y, z0)))
powers.append((t, beam_power))

for i in range(n_tracks):
    z = z0 + i*hatch
    xa, xb = (x_min, x_max) if i % 2 == 0 else (x_max, x_min)

    if points[-1][1] != (xa, beam_y, z):
        points.append((t, (xa, beam_y, z)))
        powers.append((t, beam_power))

    t += track_time
    points.append((t, (xb, beam_y, z)))
    powers.append((t, beam_power))

    if i == n_tracks - 1:
        break

    next_z = z0 + (i+1)*hatch
    t_off = t + eps
    points.append((t_off, (xb, beam_y, z)))
    powers.append((t_off, 0.0))

    t += jump_time
    points.append((t, (xb, beam_y, next_z)))
    powers.append((t, 0.0))

    t_on = t + eps
    points.append((t_on, (xb, beam_y, next_z)))
    powers.append((t_on, beam_power))
    t = t_on

with pos_file.open("w") as f:
    f.write("(\n")
    for ti, (x, y, z) in points:
        f.write(f"    ({ti:.12g} ({x:.12g} {y:.12g} {z:.12g}))\n")
    f.write(")\n")

with power_file.open("w") as f:
    f.write("(\n")
    for ti, val in powers:
        f.write(f"    ({ti:.12g} {val:.12g})\n")
    f.write(")\n")

print(f"Tracks: {n_tracks}")
print(f"End time: {t:.9g} s")
