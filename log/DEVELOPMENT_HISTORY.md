# Development history

This file is a retrospective reconstruction of the main technical path. It is
not intended to duplicate every Git commit.

## Phase 1 — define the EPBF architecture

Starting point: the incompressible `laserbeamFoam` family.

Key architectural choices:
- retain VOF free-surface/melting/flow machinery;
- represent the chamber secondary phase as numerical void rather than a
  continuum rarefied-gas model;
- separate chamber pressure from the Clausius-Clapeyron saturation reference;
- replace laser Fresnel/multiple-reflection deposition with an EPBF-specific
  electron source;
- preserve scan motion through time-position and time-power tables.

The first practical source path was deliberately simple:
`surfaceGaussian -> volumetricGaussian -> tabulatedMC` (future).

## Phase 2 — moving volumetric electron source

Implemented the current engineering EPBF source:
- transverse Gaussian profile;
- exponential depth decay;
- metal-fraction weighting;
- mesh-integrated normalization to absorbed power.

Initial surface location was too crude for deformed surfaces, leading to the
development of first-hit tracking.

## Phase 3 — multiRayFirstHit

A 5 x 12 polar beamlet map (60 beamlets) was introduced.

Purpose:
- determine local first-hit positions across the beam footprint;
- measure deposition depth from the instantaneous metal-vacuum surface;
- support future deformed surfaces and powder geometries.

Important methodological point:
the beamlets are a surface-geometry map, not a Monte-Carlo electron-transport
model.

Power conservation and hit diagnostics were added. The bare-plate calibration
later maintained 60/60 hits and absorbed-power conservation to roundoff.

## Phase 4 — phase-change convergence

Long 900 W runs exposed excessive thermal corrector counts.

Changes:
- consistent metal phase-change properties in mixed VOF cells;
- region-aware residual diagnostics;
- metal-weighted convergence criterion.

A strict 1e-4 tolerance was then tested against 1e-3.

Decision:
- accept 1e-3 for the calibration baseline;
- W/D/L unchanged at reported mesh resolution;
- fusion-volume change small;
- substantial runtime reduction.

## Phase 5 — calibration mesh

Uniform fine y-resolution was expensive.

An abrupt multi-block graded-y candidate reduced cells but produced a parallel
dynamic-mesh topology-update stall and was rejected.

Replacement:
single-block `gradedYSmooth` with smooth y-direction grading.

Decision:
- accepted;
- preserved W/D/L;
- reduced base and dynamic cells;
- improved runtime;
- avoided the abrupt internal grading transitions associated with the rejected
  candidate.

## Phase 6 — pressure-system optimization

After mesh/thermal improvements, pressure became a large cost.

Controlled A/B tests:
- GAMG/DICGaussSeidel: rejected; slower;
- PCG/FDIC: rejected; slower;
- PIMPLE nCorrectors 3 -> 2: accepted.

The 2-corrector candidate preserved W/D/L and passed the continuity gate, so
PCG/DIC + nCorrectors=2 became the numerical baseline.

Decision:
stop pressure micro-optimization and move to validation observables.

## Phase 7 — define the correct experimental observable

The original cumulative fusion-zone W/D were global whole-track bounding-box
extrema. That is not equivalent to metallographic cross-sectional width/depth.

Implemented station-wise cumulative fusion-zone sections at:
- -0.5 mm;
- 0 mm;
- +0.5 mm.

Validation path:
1. 0.05 mm I/O smoke test — CSV path passed but no melting;
2. 0.25 mm positive-signal test — central section became non-zero and matched
   the simultaneous symmetric global W/D;
3. station-wise diagnostic accepted for full-track comparison.

## Phase 8 — first full 3 mm quantitative reference

Condition:
- Ti-6Al-4V;
- 296 K;
- 900 W;
- 3 m/s;
- 3 mm;
- absorptivity 0.85;
- beam radius 250 um;
- penetration depth 18.9 um.

The first attempt was interrupted near 0.309 ms after an MPI peer reset during
a dynamic-refinement episode. Memory and disk evidence did not indicate OOM or
storage exhaustion.

Methodological response:
- add failure diagnostics;
- distinguish wrapper health from MPI-rank health;
- verify a common decomposed checkpoint;
- resume from 0.3 ms instead of restarting from zero.

The resumed run reached `End`.

Primary final result:
- central-window W = 439.354 um;
- central-window D = 67.250 um;
- experiment W = 525 um;
- experiment D = 51 um;
- width error = -16.31%;
- depth error = +31.86%.

Interpretation:
the current Gaussian source is too narrow and too deep.

## Phase 9 — long-track performance diagnosis

The full run exposed severe rank imbalance as the moving AMR region evolved.

Final state:
- global cells = 2,896,318;
- local min/max = 14,375 / 237,982;
- max/mean imbalance = 3.944.

Conclusion:
the original static Scotch decomposition is not an acceptable campaign
baseline for many long tracks, even though it can complete the calculation.

Current experiment:
compare the accepted 0.5 mm Scotch baseline against a geometry-aware
`simpleXZ` partition:

```text
n = (8 1 6)
```

Only decomposition changes.

If the 0.5 mm result is promising, confirm on a 1 mm track before promotion.

## Current position

Numerical baseline:
- phase-change tolerance frozen;
- smooth graded mesh frozen;
- pressure solver frozen;
- PIMPLE corrector count frozen;
- station-wise validation observable frozen.

Open gates:
1. decomposition/load-balance strategy;
2. depth-direction mesh sensitivity;
3. beam-radius / penetration-depth source sensitivity;
4. multi-condition Zakirov validation;
5. later acceleration-voltage-dependent / tabulated-MC deposition.
