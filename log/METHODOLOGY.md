# Development and validation methodology

## 1. Research objective

The project goal is not merely to make a laser-PBF solver run under vacuum-like
boundary conditions. The aim is to build an EPBF-specific OpenFOAM solver whose
heat deposition, free-surface interaction and validation workflow can evolve
from an engineering equivalent source to an electron-transport-based source.

Current model hierarchy:

```text
laserbeamFoam baseline
        ↓
EPBF volumetric Gaussian source
        ↓
moving free-surface multiRayFirstHit geometry
        ↓
bare-plate quantitative calibration
        ↓
voltage/material-dependent source
        ↓
tabulated Monte-Carlo electron deposition
        ↓
powder-bed EPBF
```

The current production heat source is still an equivalent model:

```text
Q ∝ alpha_metal
    exp(-2 r^2 / rb^2)
    exp(-s / penetrationDepth)
```

with global normalization to the effective absorbed beam power. The
`multiRayFirstHit` layer maps this kernel to the instantaneous VOF
metal-vacuum surface. It is a geometry layer, not a Monte-Carlo electron
transport model.

## 2. Separate four kinds of uncertainty

Every development decision should state which uncertainty it targets:

### A. Physics-model uncertainty
Examples:
- absorptivity;
- beam radius;
- penetration depth;
- recoil/evaporation/Marangoni formulation;
- Gaussian vs tabulated-MC deposition.

### B. Numerical-discretization uncertainty
Examples:
- mesh spacing;
- dynamic refinement;
- phase-change convergence tolerance;
- PIMPLE pressure-correction count;
- Courant-controlled time step.

### C. Parallel/performance uncertainty
Examples:
- decomposition strategy;
- dynamic-AMR load imbalance;
- linear-solver cost;
- reconstruction/restart overhead.

### D. Observable/validation uncertainty
Examples:
- whole-track bounding box vs metallographic cross section;
- start/stop transient contamination;
- section location;
- experimental measurement definition.

Do not tune a physics parameter to compensate for a numerical or observable
error.

## 3. Gate-based workflow

A new feature or parameter change moves through progressively more expensive
gates.

### Gate 0 — compile / syntax
Build succeeds; dictionaries parse.

### Gate 1 — smoke test
Very short run verifies control flow, MPI, I/O and diagnostics. It does not
claim physical validity.

### Gate 2 — positive-signal test
The new diagnostic/model must produce a non-zero physically interpretable
signal.

### Gate 3 — short controlled A/B
Change one variable only. Compare:
- geometry;
- conservation;
- convergence;
- performance;
- stability.

### Gate 4 — intermediate moving-track test
Used when the failure mode may accumulate with scan distance, e.g. dynamic-AMR
load imbalance.

### Gate 5 — full quantitative reference
Only after earlier gates pass. Compare the correct experimental observable.

### Gate 6 — multi-condition validation
A model is not considered calibrated because it fits one W/D pair. It must
reproduce trends across speed, preheat and later accelerating voltage.

## 4. One-change A/B principle

For numerical/performance decisions, keep all accepted settings fixed and
change one factor.

Examples already used:
- epsilon tolerance: 1e-4 → 1e-3;
- mesh: uniformFine → gradedYSmooth;
- pressure solver: PCG/DIC → GAMG;
- pressure preconditioner: DIC → FDIC;
- PIMPLE nCorrectors: 3 → 2;
- decomposition: scotch → simpleXZ.

A change is accepted only if the target metric improves without violating the
physics/conservation envelope.

## 5. Primary observable

For the 3 mm Zakirov bead-on-plate calibration, the primary observable is not
the global cumulative fusion-zone bounding box.

Use the final cumulative fusion-zone width/depth at central-track stations:

```text
x = -0.5 mm
x =  0.0 mm
x = +0.5 mm
```

and report:
- central-window mean W/D;
- station-to-station spread.

The global fusion-zone envelope remains a secondary regression diagnostic.

## 6. Numerical baseline discipline

Once a numerical choice is accepted, freeze it for physics comparisons unless
a dedicated numerical-sensitivity test reopens it.

Current accepted baseline before the decomposition study:

```text
gradedYSmooth mesh
epsilonTolerance = 1e-3
PCG + DIC
PIMPLE nCorrectors = 2
48 physical MPI cores
OMP_NUM_THREADS = 1
```

This prevents source-physics conclusions from being confounded by simultaneous
solver changes.

## 7. Long-run recovery

Expensive EPBF tracks must be restartable.

Rules:
- never discard a valid common decomposed checkpoint merely because an MPI job
  failed;
- verify the same latest written time across all ranks;
- preserve `processor*`, cumulative fields and postProcessing data;
- resume from the latest common write;
- archive the failed log;
- distinguish wrapper PID health from actual MPI-rank health.

The first 3 mm reference demonstrated why this matters: the run was recovered
from the 0.3 ms checkpoint rather than repeated from zero.

## 8. Performance before campaigns

A performance defect that scales with scan distance must be fixed before a
multi-condition campaign.

The first completed 3 mm case ended with:
- 2,896,318 cells;
- local cells 14,375 to 237,982;
- max/mean cell imbalance ≈ 3.94.

Therefore decomposition/load balancing is a campaign gate, not cosmetic
optimization.

## 9. Model-calibration rule

Do not fit absorptivity, beam radius and penetration depth simultaneously to
one W/D pair.

The first full reference gives a directional mismatch:

```text
W_sim < W_exp
D_sim > D_exp
```

The next source study should first determine sensitivities to beam radius and
penetration depth while keeping absorptivity fixed, after mesh/decomposition
uncertainty is controlled.

## 10. Upgrade criterion for tabulated MC

Do not replace the Gaussian source simply because MC is more sophisticated.

Upgrade when the validated Gaussian model reaches a demonstrated limitation,
for example:
- cannot reproduce speed/preheat/voltage trends with one physically defensible
  parameterization;
- requires condition-by-condition penetration retuning;
- fails when local incidence geometry or powder morphology matters.

Then `tabulatedMC` should replace only the deposition kernel while reusing:
- scan tables;
- multiRay surface mapping;
- power accounting;
- VOF/free-surface coupling;
- diagnostics.
