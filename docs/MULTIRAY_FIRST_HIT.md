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


## Validation status

Validated on 2026-09-18 with OpenFOAM v2512 using the
`barePlate_singleTrack` tutorial on 16 MPI ranks.

Test horizon: 100 microseconds.

Observed diagnostics at 100 microseconds:

```text
surface tracking     = multiRayFirstHit
beamlets             = 60
beamlets hit metal   = 60
hit power fraction   = 1
first-hit mean dist  = 5.426733688e-05 m
first-hit min dist   = 3.437493e-05 m
first-hit max dist   = 6.874993e-05 m
incident power       = 156 W
requested absorbed   = 132.6 W
effective absorbed   = 132.6 W
integrated deposited = 132.6 W
power error          = 3.694822226e-13 W
```

The run completed normally with `End` after about 991 s wall-clock time.
Across all five write times (20, 40, 60, 80 and 100 microseconds), all
60 beamlets hit metal and the hit power fraction remained 1. The first-hit
distance spread increased as the free surface evolved, which is the expected
qualitative behaviour of a local multi-ray surface map.

The build completed successfully on OpenFOAM v2512. Remaining compiler
messages are pre-existing/non-fatal warnings (for example the dependency
warning involving `alphaEqn.H` and unused recoil/evaporation coefficients).


## Hit-map cache / retrace optimisation

The multi-ray tracer is now optionally cached because late-time CFD steps can
fall into the 1e-8 to 1e-7 s range, making a full 60-beamlet trace every source
update unnecessarily expensive.

New controls:

```
multiRayCacheEnabled          true;
multiRayRetraceInterval       10;
multiRayRetraceDistanceFactor 0.05;
```

A new trace is forced when any of the following is true:

1. the cache is disabled;
2. no valid hit map exists yet;
3. `multiRayRetraceInterval` source updates have elapsed; or
4. the beam centre has moved by at least
   `multiRayRetraceDistanceFactor * beamRadius`.

Setting `multiRayCacheEnabled false` restores the already validated
every-update retracing algorithm and is the reference mode for A/B testing.

Write-time diagnostics now include:

```text
hit-map retraced
hit-map cache age
```

This optimisation is deliberately on `develop` until a cached-vs-reference
100 microsecond comparison confirms that the melt-pool/source diagnostics are
unchanged within an acceptable tolerance and wall-clock time improves.
