# Dynamic load-balancing plan

## Motivation

Static decomposition has reached a clear limitation.

Evidence:

- historical 3 mm Scotch final max/mean imbalance: ~3.94;
- simpleXZ 8x1x6:
  - 0.5 mm: 1.683;
  - 1.0 mm: 2.625;
- simpleXZ 4x1x12 at 0.5 mm:
  - imbalance improved to 1.401;
  - ClockTime nevertheless worsened by ~33%.

Therefore the next question is not "which static partition looks prettiest?"
but:

> Can an evolved dynamic-AMR checkpoint be safely repartitioned and resumed,
> while preserving the physical solution and reducing post-checkpoint wall
> cost?

## OpenFOAM mechanism

OpenFOAM provides `redistributePar` for redistributing an already decomposed
mesh and fields according to the current `decomposeParDict`. It runs in
parallel and must use the maximum number of source/destination processors.

The first project test will use it only at an explicit write checkpoint. This
is **checkpoint rebalancing**, not in-solver automatic dynamic load balancing.

## Gate design

Use a 1 mm moving-track case because an unrebalanced reference already exists.

### Reference

Existing 1 mm simpleXZ 8x1x6 result:

```text
final imbalance = 2.625
ClockTime       = 24059 s
x=0 section     = 435.187 / 67.250 um
```

### Rebalanced experiment

1. Generate the same full 1 mm beam path.
2. Start from the same 8x1x6 decomposition.
3. Stop at an exact normal write checkpoint near the track midpoint.
4. Save the pre-redistribution diagnostics.
5. Preserve a rollback copy/snapshot of the checkpoint.
6. Change only the decomposition method used for redistribution.
7. Run `redistributePar` in parallel on the checkpoint.
8. Verify:
   - 48 processor domains exist;
   - latest time is common across all ranks;
   - mesh/fields are readable;
   - cumulative fields such as `everMelted` and `peakTemperature` are present.
9. Resume the same beam path from the checkpoint to the original 1 mm end time.
10. Compare against the unrebalanced 1 mm reference.

## Quantities to compare

Physics:
- x=0 fusion-zone W/D;
- global fusion-zone W/D/L/V;
- beam power conservation;
- continuity.

Parallel:
- local cell min/max immediately before and after redistribution;
- max/mean imbalance;
- heaviest rank;
- post-checkpoint wall/step;
- pressure and thermal wall/step;
- total continuation ClockTime.

Robustness:
- dynamicRefineFvMesh reads the redistributed topology;
- no missing fields;
- no MPI peer failure;
- restart reaches End.

## Acceptance logic

A redistribution workflow is promising only if:

1. physical observables remain within the accepted numerical envelope;
2. the checkpoint cell imbalance drops materially;
3. post-checkpoint wall cost drops enough to repay redistribution overhead;
4. the continuation remains stable.

Do not choose a fixed automatic imbalance threshold yet. First measure one or
more controlled checkpoint/restart experiments.

## Safety rules

- Never repartition the only copy of an expensive checkpoint without rollback.
- Never restart from t=0 merely because redistribution fails.
- Keep source/physics/numerical settings frozen during this gate.
- Archive pre/post redistribution decomposition statistics.
- Treat redistribution time itself as campaign overhead and include it in the
  performance comparison.

## Next implementation step

Create a staged 1 mm runner that can:
- stop at the midpoint write;
- archive/verify the checkpoint;
- execute a dry-run/real redistribution safely;
- resume without cleaning processor directories or cumulative diagnostics.

Only after that runner passes should checkpoint redistribution be considered
for full 3 mm or six-condition campaign use.
