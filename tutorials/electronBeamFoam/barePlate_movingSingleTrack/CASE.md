# Moving single-track EPBF calibration scaffold

This case is the first moving-source bare-plate tutorial for quantitative EPBF
calibration. It is deliberately separated from `barePlate_singleTrack`,
which remains the short stationary-spot regression case.

## Default geometry and scan

The mesh is 1.0 mm x 0.5 mm x 0.5 mm with the same 12.5 micrometre coarse
cell size used by the stationary case:

- x: scan direction, -0.5 to +0.5 mm;
- y: beam/depth direction, 0 to 0.5 mm;
- z: transverse direction, 0 to 0.5 mm;
- initial free surface: y = 0.1 mm.

The default source moves from x = -0.35 mm to +0.35 mm at 0.5 m/s with
156 W incident power. These values are placeholders inherited from the
validated source case; they are not yet a calibrated Ti-6Al-4V EPBF set.

## Parameterised runs

The runner accepts environment overrides without permanently changing tracked
OpenFOAM dictionaries:

```bash
BEAM_POWER=180 \
SCAN_SPEED=0.7 \
X_START=-0.00035 \
X_END=0.00035 \
WRITE_INTERVAL=1e-4 \
NPROCS=48 \
./Run_background
```

Optional variables are `BEAM_Y`, `BEAM_Z` and `COOL_TIME`.

`generateMovingSingleTrack.py` generates the beam position/power tables and
updates the temporary run end time. `Allrun_parallel` restores the tracked
inputs on exit, so normal parameter sweeps do not leave `controlDict` or beam
tables modified in Git.

## Quantitative outputs

The solver writes:

```text
postProcessing/meltPoolDiagnostics/meltPool.csv
```

containing time, melt-pool length, width, depth, volume, Tmax, Umax and maximum
recoil pressure. The same run also prints stage-level performance profiling.

The next step is to replace the placeholder beam/material inputs with a
documented Ti-6Al-4V EPBF single-track calibration target.


## Large solver logs

Long moving-track runs can produce very large `log.electronBeamFoam` files
because the temperature/phase-change correctors print every linear solve.
For routine review, do not copy the full log. Generate a compact diagnostic
summary instead:

```bash
./SummarizeRun
```

This writes `run-summary.txt` with beam diagnostics, melt-pool diagnostics,
performance profiles, runtime and termination status. Keep the full solver log
locally for debugging.
