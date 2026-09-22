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
epsilonTolerance 1e-4;
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
