# 2026-09-28 — simpleXZ 1 mm confirmation

## Question

Did the promising 0.5 mm `simpleXZ (8 1 6)` decomposition remain sufficiently
balanced as the moving dynamic-refinement region travelled farther?

## Controlled change

No physics or numerical settings changed from the accepted baseline.

The run used:
- 296 K;
- 900 W;
- 3 m/s;
- 1 mm track;
- gradedYSmooth;
- epsilonTolerance=1e-3;
- PCG/DIC;
- nCorrectors=2;
- 48 ranks;
- simple decomposition n=(8 1 6).

## Result

The solver reached `End`.

The central x=0 cumulative section at the final normal write was:

```text
W = 435.187 um
D = 67.250 um
```

which exactly matches the completed 3 mm baseline central station at the
reported mesh resolution.

Beam accounting remained 60/60 with unit hit fraction and deposited power
conserved to roundoff. Continuity remained small.

The load-balance trajectory, however, deteriorated with scan distance:

```text
0.5 mm final max/mean imbalance = 1.683
1.0 mm final max/mean imbalance = 2.625
growth factor                  = 1.560x
```

At the 1 mm final write:

```text
global cells        = 2,092,277
local min/max       = 37,620 / 114,431
max/min             = 3.042x
heaviest rank       = 27
ClockTime           = 24,059 s
```

Intermediate profiles show the same monotonic trend:
- ~0.1 ms: imbalance 1.494;
- ~0.2 ms: imbalance 2.311;
- ~0.3 ms: imbalance 2.625.

Therefore the 0.5 mm improvement is real but does not remain sufficiently flat
with track length.

## Interpretation

The 8x1x6 strategy passes the **physics and short-run performance** checks, but
does not fully solve the campaign-scale load-balance problem.

The main structural weakness is that the refined moving track occupies only a
small subset of surface-plane partitions at a time. A new static candidate will
shift more partitions into the transverse z direction while still leaving every
rank uncut in y:

```text
simple n=(4 1 12)
```

This is a hypothesis, not an accepted improvement. It keeps 48 equal base
partitions because 176/4 and 60/12 are both exact integers.

The reasoning is that the narrow melt-track/refined band can be distributed
across more z partitions, potentially reducing the peak local refined-cell
accumulation observed in the 8x1x6 case.

## MPI warning caveat

The final compact Status contained historical OpenMPI TCP
`Connection reset by peer` signatures even though the run reached `End`.
The supplied compact outputs do not establish where those messages occurred in
the raw log, so they are recorded as a warning rather than used as evidence of
a failed 1 mm run.

## Decision

**Do not promote 8x1x6 as the final campaign decomposition.**

Retain it as a demonstrated improvement over the original long-track Scotch
behaviour and as the decomposition benchmark for the next static-partition
test.

## Next gate

Run a 0.5 mm controlled A/B using `simple (4 1 12)`.

Accept it for a 1 mm confirmation only if:
1. W/D/L remain inside the accepted envelope;
2. actual wall time remains competitive;
3. max/mean imbalance is lower than the 8x1x6 0.5 mm result;
4. no new stability problem appears.

If static surface-plane decompositions still show rapid imbalance growth at
1 mm, stop searching static layouts and move to checkpoint redistribution /
dynamic load-balancing workflow design.
