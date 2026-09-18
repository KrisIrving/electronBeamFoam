# multiRayFirstHit development update

This development commit advances electronBeamFoam from the validated
central-axis first-hit model to a parallel polar beamlet first-hit surface map.

## Model

- The Gaussian beam is sampled with `beamletRadialBins * beamletAngularBins`
  beamlets (default 5 x 12 = 60).
- Each beamlet uses OpenFOAM Lagrangian face-to-face tracking and processor
  transfer, and terminates at the first cell satisfying
  `alpha.metal >= firstHitAlphaCutoff`.
- Beamlet hit records are gathered and broadcast so every MPI rank reconstructs
  the same local first-hit surface map.
- The deposited heat source remains a smooth volumetric Gaussian. Each CFD cell
  maps to one polar beamlet sector, so the deposition loop remains O(Ncells)
  rather than O(Nbeamlets*Ncells).
- The beamlet hop counter is serialized with the particle, so the configured
  hop limit remains valid across processor transfers.
- With `beamletRadiusFactor 2.0`, the sampled footprint contains more than
  99.96% of an ideal Gaussian beam.

## New controls

```
surfaceTrackingMode     multiRayFirstHit;
beamletRadialBins       5;
beamletAngularBins      12;
beamletRadiusFactor     2.0;
beamletMaxTrackHops     100000;
multiRayMissAction      skip;
```

## Required validation

First rebuild:

```bash
./Allwmake > log.build.multiRay 2>&1
```

Then use the single-track bare-plate tutorial for a short 100 microsecond run.
On the initially flat plate, all 60 beamlets should normally hit metal and the
deposited-power error should be near roundoff.

This commit intentionally does not yet cache the hit map. Correctness should be
verified before adding retrace/caching optimisation.
