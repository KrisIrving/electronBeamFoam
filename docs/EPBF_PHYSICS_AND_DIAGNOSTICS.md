# EPBF physics configuration and melt-pool diagnostics

This stage turns the validated electron-beam source into a more explicit EPBF
melt-pool model and establishes quantitative outputs for calibration.

## Vacuum representation

`vacuumModel numericalVoid` is the only supported chamber representation in
this stage. The secondary VOF phase exists for numerical interface tracking and
pressure coupling. It must not be interpreted as a continuum rarefied-gas
solution.

The Clausius-Clapeyron saturation reference `p0` remains independent of the
chamber pressure `pAmbient`.

## Physics switches

`constant/EPBFPhysicsProperties` controls:

```text
radiation
evaporation
recoilPressure
marangoni
surfaceTension
gravity
electromagnetics
```

The default bare-plate tutorials keep the established thermal-capillary,
evaporation and recoil physics enabled. Electromagnetics is disabled unless an
MHD model is intentionally configured.

## Melt-pool definition

At each normal OpenFOAM write time, a cell is included in the diagnostic melt
pool when both conditions are met:

```text
alpha.metal >= metalFractionThreshold
epsilon1    >= liquidFractionThreshold
```

The thresholds are deliberately configurable. For future comparison with
experimental fusion boundaries, sensitivity to the liquid-fraction threshold
must be reported rather than hidden.

The diagnostics produce:

- projected melt-pool length;
- projected melt-pool width;
- depth from the configured original free-surface reference plane;
- liquid-metal volume;
- maximum temperature;
- maximum velocity magnitude;
- maximum recoil pressure.

A half-cell correction based on the local equivalent cell size reduces the
systematic centre-sampling bias on the dynamically refined mesh.

Results are printed in the solver log and appended to:

```text
postProcessing/meltPoolDiagnostics/meltPool.csv
```

## Current coordinate convention

For the provided bare-plate tutorials:

```text
lengthDirection       (1 0 0)
widthDirection        (0 0 1)
depthDirection        (0 1 0)
referenceSurfacePoint (0 0.0001 0)
```

The current single-track tutorial is still a stationary-spot validation case.
A true moving single-track EPBF calibration case will be introduced next.
