# 2026-09-27 — full-reference baseline to decomposition campaign

## Question

After the first completed 3 mm quantitative EPBF reference, what should be fixed
before launching a multi-condition physical calibration campaign?

## Evidence from the full reference

Physics result:
- central-window W = 439.354 um vs 525 um experiment;
- central-window D = 67.250 um vs 51 um experiment;
- the source therefore predicts a track that is too narrow and too deep.

Validation quality:
- three central sections have only 6.25 um width spread;
- depth is identical at the reported resolution;
- beam hit fraction is 1 and deposited power is conserved;
- continuity remains small.

Performance result:
- final mesh = 2,896,318 cells;
- local rank cells = 14,375 ... 237,982;
- max/mean imbalance = 3.944;
- the full run became prohibitively expensive for a six-condition campaign.

## Methodological interpretation

Do not immediately tune beam radius or penetration depth.

The observed W/D mismatch contains at least three separable questions:

1. Is the long-track parallel decomposition distorting campaign cost?
2. Is D=67.25 um partly quantised by the present near-surface y resolution?
3. After those are controlled, how does the equivalent source respond to
   beam radius and penetration depth?

If source parameters are tuned before (1) and (2), runtime and discretisation
effects become mixed with source physics.

## Decision

First perform a decomposition A/B with all physics and numerics frozen.

Candidate:

```text
simple decomposition
n = (8 1 6)
```

Rationale:
- 176 x base cells divide exactly by 8;
- 60 z base cells divide exactly by 6;
- every rank spans the full graded-y direction;
- the metal/vacuum interface should initially be spread more uniformly across
  ranks.

The first gate is 0.5 mm. If it reduces both imbalance and actual wall time
without changing W/D/L, confirm it on 1 mm before replacing Scotch.

## Next steps after decomposition

1. y/depth mesh sensitivity;
2. beam-radius sensitivity at fixed absorptivity;
3. penetration-depth sensitivity at fixed absorptivity;
4. select a small number of candidates for full 3 mm verification;
5. expand to Zakirov speed/preheat family;
6. only then decide whether the Gaussian source has reached its evidence-based
   limit and should be upgraded to tabulated MC.
