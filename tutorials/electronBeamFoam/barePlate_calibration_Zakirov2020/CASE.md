# Zakirov2020 Ti-6Al-4V bead-on-plate calibration

This case is the first quantitative experimental validation case for
`electronBeamFoam`.

Reference:
Andrey Zakirov et al., Additive Manufacturing 35 (2020) 101236,
DOI 10.1016/j.addma.2020.101236.

## Default experimental target

The default run corresponds to the 296 K, 60 kV, 15 mA bead-on-plate data:

- incident power: 900 W;
- scan speed: 3000 mm/s (3.0 m/s);
- line energy: 0.30 J/mm;
- nominal D4sigma beam diameter: 500 um;
- experimental fusion width: 525 um;
- experimental fusion depth: 51 um.

For the source convention `exp(-2 r^2/rb^2)`, the initial nominal Gaussian
radius is `rb = D4sigma/2 = 250 um`. Initial absorptivity is 0.85 and the
initial exponential penetration scale is 18.9 um.

These are starting values for validation, not fitted values.

## Domain and mesh

The domain is:

- x = -2.2 ... +2.2 mm (scan direction);
- y = 0 ... 0.6 mm (beam/depth direction);
- z = 0 ... 1.5 mm (transverse direction);
- initial free surface y = 0.2 mm;
- beam seed plane y = 0.05 mm;
- beam centre z = 0.75 mm.

Base cell sizes are approximately:

- dx = 25 um;
- dy = 6.25 um;
- dz = 25 um.

The existing two dynamic-refinement levels refine the metal/vacuum interface
by another factor of four. The globally finer y spacing is intentional because
the experimental fusion depth is only 51 um.

The reference numerical track length is 3 mm, matching the track length used
in the source paper's numerical bead calculations.

## Phase-change model

This case uses the validated moving-case settings:

```text
consistentMetalPhaseChange true;
phaseChangeMetalFractionCutoff 0.01;
epsilonTolerance 1e-3;
phaseChangeResidualMode metalWeightedMax;
```

The preceding A/B regression reduced thermal corrector cost without changing
fusion-zone L/W/D at the available mesh resolution.

## Running

Optional short preflight:

```bash
./Run_preflight
```

uses a 0.5 mm track only to check the larger calibration mesh, source footprint,
beam hits and solver stability. It must not be treated as the final
experimental W/D comparison.

Production reference:

```bash
./Run_background
```

uses the full 3 mm track and 48 MPI ranks with 24 physical cores per socket.

The generator supports source-sensitivity overrides, for example:

```bash
BEAM_RADIUS=300e-6 \
PENETRATION_DEPTH=20e-6 \
ABSORPTIVITY=0.85 \
./Run_background
```

It also supports `PREHEAT_K`, `BEAM_POWER`, `SCAN_SPEED`, `X_START`,
`X_END`, `BEAM_Y`, `BEAM_Z`, `COOL_TIME`, and `WRITE_INTERVAL`.
Tracked input dictionaries are restored after each run.

## Outputs

Use:

```bash
./Status
./SummarizeRun
```

Primary calibration output:

```text
postProcessing/meltPoolDiagnostics/fusionZone.csv
```

`CompareExperiment.py` reads the final cumulative fusion width/depth and
matches the run against `validation/Zakirov2020_Ti64_beadOnPlate.csv`.

A completed default run therefore reports directly:

```text
Wsim vs Wexp=525 um
Dsim vs Dexp=51 um
```

Do not fit absorptivity, beam radius and penetration depth simultaneously from
this single condition. This case establishes the first baseline; the 1000 and
6000 mm/s conditions and the 973 K preheat family are used to constrain trends.


## High-power convergence A/B

The first 900 W / 3 m/s / 0.5 mm preflight completed normally but was much
more expensive than the low-power regression. In the developed second half,
the metal-weighted convergence residual again exceeded 1e-4 in many interface
cells and roughly 60% of temperature solves reached maxTempCorrector.

Before the full 3 mm production run, use:

```bash
./Run_toleranceProbe
```

This archives the current 1e-4 preflight under
`comparisons/preflight_tol1e-4` and repeats the identical 0.5 mm trajectory
with `EPSILON_TOLERANCE=1e-3`.

After completion:

```bash
./SummarizeRun
./CompareTolerance.py
```

Accept the looser tolerance only if the fusion-zone W/D shift remains within
the available mesh resolution while thermal cap hits and wall time decrease
materially. The full 3 mm calibration should not be started before this A/B
gate is resolved.


## Candidate graded-y calibration mesh

The first 900 W preflight reached more than 2.2 million dynamically refined
cells. The reference base mesh uses dy=6.25 um through the full 0.6 mm domain,
although the experimental fusion depth is only about 51 um.

A candidate mesh is now available as:

```text
system/blockMeshDict.gradedY
```

It keeps the x/z domain and discretization unchanged but splits y into:

```text
0.00 - 0.15 mm : dy = 12.5 um   (vacuum/headspace)
0.15 - 0.35 mm : dy =  6.25 um  (surface + top 150 um substrate)
0.35 - 0.60 mm : dy = 25.0 um   (deep substrate)
```

The initial surface y=0.20 mm lies exactly on a fine-band cell face. The fine
band extends 150 um below the surface, comfortably beyond the current
61 um preflight fusion depth.

Base-cell count falls from 1,013,760 to 570,240 (-43.75%). The candidate is
not yet the production default.

After the 1e-3 tolerance A/B is accepted, run:

```bash
./Run_meshProbe
```

Then:

```bash
./SummarizeRun
./CompareMesh.py
```

This performs the same 0.5 mm / 900 W / 3 m/s trajectory with
`MESH_PROFILE=gradedY` and compares fusion-zone geometry, final cell count and
wall time against the archived uniform-fine 1e-3 result.


## Tolerance A/B accepted

The 0.5 mm, 900 W, 3 m/s A/B test changed epsilonTolerance from 1e-4 to
1e-3 with the following result:

- fusion-zone width: unchanged at the reported mesh resolution;
- fusion-zone depth: unchanged at the reported mesh resolution;
- fusion-zone length: unchanged at the reported mesh resolution;
- fusion-zone volume: +0.184%;
- ClockTime: 19112 s -> 14091 s (1.356x speedup);
- developed-stage thermal cap hits: 2614 -> 59.

The calibration default is therefore now epsilonTolerance=1e-3. The raw and
metal-weighted residual diagnostics remain enabled so future high-power runs
can expose any loss of convergence.


## Duplicate-run protection

Calibration runs are destructive to the case-local `processor*`,
`postProcessing` and log directories. Starting two runs in the same case
directory is therefore invalid.

`Allrun_parallel` now holds an advisory `flock` for its full lifetime.
`Run_background`, `Run_toleranceProbe` and `Run_meshProbe` refuse to
launch or archive data while that lock is held.

The accepted 0.5 mm / epsilonTolerance=1e-3 / uniformFine mesh baseline is
also committed as:

```text
validation/Zakirov2020_preflight_tol1e-3_uniformFine.json
```

so mesh comparisons no longer depend on a mutable local
`comparisons/` directory.


## Parallel AMR stall in abrupt gradedY

The first `gradedY` candidate used abrupt y-cell-size jumps
(12.5 -> 6.25 -> 25 um) across three block interfaces. During the 900 W
preflight it advanced normally to about 7.53e-05 s, then stopped returning from
a dynamicRefineFvMesh topology update immediately after reporting refine and
unrefine operations. MPI ranks remained alive and mostly busy, while the solver
log stopped advancing for more than an hour.

That candidate is retained for provenance but is no longer used by
`Run_meshProbe`.

The replacement `gradedYSmooth` is a single block with multi-grading in y.
It preserves a 6.25 um near-surface band but transitions smoothly to coarser
far-field cells:

```text
0.00 - 0.15 mm : 16 graded cells, ~13.3 -> 6.25 um
0.15 - 0.35 mm : 32 uniform cells, 6.25 um
0.35 - 0.60 mm : 18 graded cells, 6.25 -> ~25.7 um
```

It has 696,960 base cells, a 31.25% reduction from the 1,013,760-cell
uniformFine reference. The initial surface remains exactly on a cell face.

`Status` now also reports solver-log age and warns when an active process has
not written for five minutes. If the last lines are AMR refine/unrefine
messages, it identifies dynamic mesh topology update/field mapping as the
suspect stage.


## gradedYSmooth accepted

The smooth graded-y candidate completed the full 0.5 mm high-power preflight
without the AMR stall seen in the abrupt multi-block candidate.

Compared with the accepted uniformFine / epsilonTolerance=1e-3 baseline:

- fusion-zone W/D/L: unchanged at reported resolution;
- fusion-zone volume: -0.097%;
- final global cells: 2,241,196 -> 1,923,255 (-14.19%);
- ClockTime: 14,091 s -> 9,626 s (1.464x speedup);
- developed-stage thermal cap hits: 59 -> 7.

The base mesh itself contains 696,960 cells (-31.25% vs uniformFine).
The mesh passed all checkMesh topology and geometry checks, including zero
non-orthogonality and maximum aspect ratio 4.

`gradedYSmooth` is therefore the calibration default mesh profile. The
accepted result is committed under validation for subsequent pressure-solver
A/B tests.


## Pressure-solver A/B

After accepting `gradedYSmooth`, pressure is the dominant remaining solver
cost: about 42% of the developed-stage wall time in the 900 W preflight.

The reference remains:

```text
p_rgh: PCG + DIC
tolerance 1e-7
relTol 0.05
```

An optional GAMG profile is provided in `system/fvSolution.GAMG` with the
same absolute and relative tolerances and a DICGaussSeidel smoother.

Run the strict A/B with:

```bash
./Run_pressureProbe
```

After completion:

```bash
./SummarizeRun
./ComparePressure.py | tee pressure-comparison.txt
```

Only the pressure linear solver changes; source, mesh, phase-change settings,
track length and PIMPLE corrector count remain fixed.


## GAMG pressure probe rejected

The strict 0.5 mm pressure A/B changed only the p_rgh linear solver from
PCG/DIC to GAMG/DICGaussSeidel. Fusion-zone W/D/L were unchanged and volume
shifted by only +0.028%, but performance regressed:

- developed-stage pressure wall: 2946.9 -> 3352.5 s (+13.8%);
- developed-stage total wall: 6986.5 -> 7463.6 s (+6.8%);
- ClockTime: 9626 -> 10311 s (+7.1%).

GAMG is therefore rejected for this 48-rank calibration workload. PCG/DIC
remains the accepted baseline.

The next low-risk probe keeps PCG and all tolerances fixed, changing only the
preconditioner from DIC to FDIC. Run:

```bash
./Run_pressureFdicProbe
```

Then:

```bash
./SummarizeRun
./ComparePressureFdic.py | tee pressure-fdic-comparison.txt
```


## FDIC pressure probe rejected

The PCG/FDIC A/B reproduced the accepted PCG/DIC trajectory essentially
exactly: fusion-zone W/D/L/V, final dynamic-cell count and thermal cap hits were
unchanged. Performance nevertheless regressed:

- developed-stage pressure wall: 2946.9 -> 3051.8 s (+3.6%);
- developed-stage total wall: 6986.5 -> 7221.2 s (+3.4%);
- ClockTime: 9626 -> 9883 s (+2.7%).

PCG/DIC remains the pressure-linear-solver baseline.

The next probe targets pressure-correction count instead of the linear solver.
It keeps PCG/DIC and all pressure tolerances fixed and changes only PIMPLE
`nCorrectors` from 3 to 2. The final correction still uses `p_rghFinal`
with `relTol=0`.

Run:

```bash
./Run_pressure2CorrProbe
```

Then:

```bash
./SummarizeRun
./ComparePressure2Corr.py | tee pressure-2corr-comparison.txt
```


## Two-corrector candidate

The 0.5 mm PIMPLE A/B changed only `nCorrectors` from 3 to 2 while retaining
PCG/DIC and the strict final `p_rghFinal` solve.

Observed result:

- fusion-zone W/D/L: unchanged at reported resolution;
- fusion-zone volume: -0.151%;
- final dynamic cells: +0.03%;
- developed-stage pressure wall: 2946.9 -> 2351.8 s (1.253x);
- total ClockTime: 9626 -> 8926 s (1.078x);
- pressure correctors/step: 3.000 -> 2.000;
- thermal cap hits: 7 -> 16.

This is a strong performance candidate but is not promoted to the default until
the continuity-error gate is checked from the raw solver log. `SummarizeRun`
now extracts max local/global and cumulative continuity metrics automatically.
No rerun is required: regenerate the summary from the existing solver log.
