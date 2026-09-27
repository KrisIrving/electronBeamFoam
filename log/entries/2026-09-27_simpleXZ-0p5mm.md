# 2026-09-27 — simpleXZ 0.5 mm decomposition gate

## Question

Can a geometry-aware 48-rank partition reduce the severe long-track
dynamic-AMR imbalance seen with the Scotch campaign baseline without changing
the physical answer?

## Hypothesis

Partitioning the surface plane as `(8 1 6)` while leaving y undecomposed
should distribute the initially flat metal/vacuum interface more uniformly
across all ranks and reduce the cost of moving local refinement.

## Controlled change

Only decomposition changed:

```text
baseline: scotch
probe:    simple n=(8 1 6)
```

All accepted physics/numerics were retained:
- gradedYSmooth;
- epsilonTolerance=1e-3;
- PCG/DIC;
- nCorrectors=2;
- 48 physical ranks;
- 296 K, 900 W, 3 m/s, 0.5 mm.

## Evidence

`decomposePar` reported `simple [48]` and exactly 14,520 base cells on every
rank.

Final fusion zone:

```text
W 435.187 -> 435.187 um   0.000%
D 60.9995 -> 60.9995 um  0.000%
L 685.187 -> 685.187 um  0.000%
V 0.00582601 -> 0.00581220 mm3  -0.237%
```

Final simpleXZ partition state:

```text
global cells           1,919,909
local cells min/max    37,620 / 67,300
max/mean imbalance     1.683
```

Performance:

```text
ClockTime              8926 -> 4260 s   2.095x
last-interval wall     6535 -> 2979 s   2.194x
pressure wall          2352 -> 1059 s   2.222x
thermal wall           1331 -> 593 s    2.242x
```

The final interval also used 2671 steps instead of 3589 (-25.6%). Therefore
the raw ~2.1x acceleration is not purely an MPI per-step gain. Normalised per
time step, the total wall cost still improves by about 1.63x, with pressure and
thermal costs improving by about 1.65x and 1.67x respectively.

Continuity remained small and there were no thermal cap hits.

## Interpretation

The 0.5 mm gate is a strong pass:
- geometry is unchanged at reported resolution;
- volume shift is small;
- the final rank distribution is much tighter than the severe long-track
  Scotch state;
- the speedup remains substantial even after normalising for fewer time steps.

The fewer time steps also show that floating-point / partition-order effects
alter the detailed timestep trajectory. This is acceptable for a performance
probe only because W/D/L and conservation remain inside the accepted envelope;
it is one reason not to promote the decomposition from a 0.5 mm test alone.

## Decision

**ACCEPT at the 0.5 mm gate; ACTIVE for 1 mm confirmation.**

Do not yet replace Scotch as the campaign default.

## Next gate

Run a 1 mm simpleXZ track. Confirm:
1. max/mean imbalance does not rapidly re-grow;
2. solver remains stable;
3. central x=0 section is consistent with the developed full-track baseline;
4. per-step and total performance remain favorable.
