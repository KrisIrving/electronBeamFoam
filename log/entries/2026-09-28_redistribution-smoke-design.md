# 2026-09-28 — checkpoint redistribution smoke-test design

## Why this gate exists

Static decomposition tests established a tradeoff:

- simple 8x1x6 is fast at 0.5 mm, but imbalance grows to 2.625 by 1 mm;
- simple 4x1x12 gives better 0.5 mm cell balance but worse wall time.

The next question is therefore not which static partition looks best at t=0,
but whether an already refined moving-AMR checkpoint can be safely
redistributed and resumed.

## Tool-level evidence

OpenFOAM redistributePar is intended to redistribute an existing decomposed
mesh and fields according to a decomposeParDict and is run in parallel.

The OpenFOAM source also explicitly handles hexRef8 refinement data via
hexRef8Data during redistribution. This matters for dynamicRefineFvMesh because
its restart state includes cellLevel, pointLevel and level0Edge.

These capabilities are treated as necessary but not sufficient evidence. The
electronBeamFoam workflow must still prove restart fidelity empirically.

## Smoke design

Use the accepted 0.5 mm physics path.

Stage 1:
- start with simpleXZ 8x1x6;
- run only to the normal midpoint write;
- require all 48 ranks to share the checkpoint time;
- require everMelted, peakTemperature, cellLevel, pointLevel and level0Edge on
  all ranks.

Safety:
- create a full tar archive of processor* before redistribution.

Redistribution:
- run 48-rank redistributePar;
- select latestTime;
- use -overwrite so the checkpoint time itself is replaced rather than a
  synthetic later time;
- target Scotch on the already refined mesh;
- audit post-distribution rank cell counts;
- require checkMesh to pass.

Stage 2:
- restore the original full 0.5 mm endTime;
- resume electronBeamFoam from latestTime;
- require new dynamic refine/unrefine activity;
- preserve postProcessing so cumulative fusion history continues.

## Why Scotch is used at the checkpoint

This does not repeat the failed campaign strategy of Scotch at t=0.

At t=0 Scotch sees a nearly uniform base mesh. At the checkpoint it sees the
actual refined mesh produced by the moving interface and beam. The graph being
partitioned therefore contains the current AMR workload.

## Acceptance

The first gate is fidelity, not speed.

Required:
- W/D/L reproduce the accepted 0.5 mm baseline;
- cumulative fusion-zone history continues across the checkpoint;
- dynamicRefineFvMesh remains active after restart;
- refinement state and cumulative fields survive redistribution;
- checkMesh passes;
- beam power and continuity remain acceptable.

Only then should a 1 mm redistributed run be used to measure whether
redistribution overhead is repaid by lower solver wall time.
