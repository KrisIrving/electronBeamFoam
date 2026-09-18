# electronBeamFoam first code drop

Derived from the uploaded LaserbeamFoam working copy:
- branch: `OpenFoam_com_main`
- commit: `7578023ce1b33f9ad7a11f0c4f82b66ce0a6621f`

## Added
- `applications/solvers/electronBeamFoam`
- `src/electronBeamHeatSource`
- `tutorials/electronBeamFoam/barePlate_singleTrack`
- separate EPBF chamber pressure (`pAmbient`) from the Clausius-Clapeyron
  saturation reference pressure (`p0`).

## Implemented source models
- `surfaceGaussian`
- `volumetricGaussian`

The deposition field is normalised each update so that:
`integral(ElectronDeposition dV) = absorptivity * beamPower`
whenever the beam intersects eligible cells.

`tabulatedMC` is reserved in the interface but deliberately not implemented yet.

## Surface-following electron deposition (2026-09-18)

The volumetric Gaussian no longer has to use the tabulated beam position as the
zero of penetration depth.  `surfaceTrackingMode centralFirstHit` now searches
for the first metal-vacuum interface intercepted by the **central beam axis**
and uses that moving interface point as the origin of the volumetric electron
deposition kernel.

This removes an important systematic error in the first implementation: when a
beam-position point was placed in the vacuum above the workpiece, the old model
measured `penetrationDepth` from that vacuum point.  Since gas cells were then
masked out and the remaining source was re-normalised, the total power was
correct but the through-depth distribution inside the metal was shifted.

Implemented modes:
- `fixedReference`: legacy behaviour, penetration measured from the tabulated
  beam-position point.
- `centralFirstHit`: MPI-safe global first-hit search around the beam axis, with
  a resolved-interface search and a first-bulk-metal fallback.

At write times the log reports `deposition origin`, `first-hit found`, and
`first-hit distance`.

### Remaining powder-bed limitation
`centralFirstHit` follows the free surface at the beam centre and is suitable for
bare plates and moderately deformed melt pools.  It is **not yet the final
powder-bed treatment**, because different transverse parts of a finite-width
beam can hit different particles/heights.  The next source upgrade is a
`multiRayFirstHit`/ray-column model that reuses the robust parallel geometric
tracking machinery already present in laserbeamFoam, while discarding Fresnel
reflection physics.

## Compile
```bash
source <your OpenFOAM environment>
cd ElectronBeamFoam-v3-dev
./Allwmake
```

This sandbox does not contain your OpenFOAM installation, so this code drop has
not been compiled here. Compile diagnostics from your workstation should be fed
back before treating the branch as build-clean.

## First checks
1. `which electronBeamFoam`
2. Copy `tutorials/electronBeamFoam/barePlate_singleTrack` to a run directory.
3. Verify `beamDirection` and reference position against the mesh orientation.
4. Run the inherited case setup.
5. Check the log: integrated deposited power should equal
   `absorptivity * incident power`.

## Vacuum pressure warning
`p0` is a saturation-pressure reference used with `Tvap`; do not set it to the
EPBF chamber pressure. `pAmbient` and `pVaporAmbient` are separate inputs.

## 2026-09-18 performance/parallel update

- Build log confirmed successful compilation with OpenFOAM v2512.
- Tutorial default changed from 8 to 32 MPI ranks (override with `NPROCS`).
- Solver and preprocessing output redirected to `log.*`; removed `tee`.
- Added `Allrun_parallel`, `Run_background`, and `Status` helpers.
- Tutorial field output changed from ASCII to binary.
- Electron source diagnostics/global integration reduced to write times.
- Added 4-beam-radius kernel cutoff and early volumetric cell rejection.
