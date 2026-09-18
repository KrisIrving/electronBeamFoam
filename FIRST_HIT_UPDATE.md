# electronBeamFoam: central first-hit update

## What changed

For `volumetricGaussian`, penetration can now be measured from the instantaneous
metal-vacuum surface instead of from the beam-path reference point in vacuum.

The source performs an MPI-safe search near the central beam axis:

1. find candidate metal/interface cells with a non-zero filtered interface normal;
2. compute positive distance along `beamDirection`;
3. reduce the minimum distance across all ranks;
4. if no interface candidate is found, optionally use the first bulk-metal cell;
5. construct the deposition origin on the central beam axis;
6. apply the existing radial Gaussian and exponential depth attenuation from this origin;
7. globally normalise the source so the integrated deposited power remains
   `absorptivity * incidentPower`.

## New ElectronBeamProperties entries

```text
surfaceTrackingMode    centralFirstHit; // fixedReference | centralFirstHit
firstHitSearchRadius   35e-6;
firstHitNormalThreshold 0.1;
firstHitAlphaCutoff    1e-3;
firstHitFallbackMetalFraction 0.5;
firstHitFallbackToMetal true;
```

For the supplied bare-plate mesh, the beam reference path is at `y=50 um` and
the initial metal surface is at `y=100 um`.  The expected first-hit distance is
therefore approximately `50 um` (subject to VOF/interface discretisation).

## Expected log diagnostic

At write times, look for:

```text
position             = (...)
deposition origin    = (...)
first-hit found      = 1
first-hit distance   = ... m
incident power       = ... W
target absorbed      = ... W
integrated deposited = ... W
power error          = ... W
```

## Build status

This update was source-checked in the sandbox but cannot be compiled here because
the sandbox does not contain OpenFOAM v2512.  The previous branch was confirmed
to compile on the user's workstation; this update changes the heat-source API,
so a workstation rebuild is required before use.

## Next development step

After this version is build-clean, implement `multiRayFirstHit`: discretise the
finite electron beam into ray columns, track each ray to its own first surface
intersection in parallel, and apply the depth-deposition law beneath each hit.
That is the version required for rough powder beds.
