# Moving-AMR load-balancing strategy

## Evidence so far

The first completed 3 mm Scotch run showed severe end-of-track imbalance:

```text
max/mean cell imbalance ~3.94
```

A geometry-aware static partition improved short-run performance:

```text
simple 8x1x6:
0.5 mm imbalance = 1.683
ClockTime = 4260 s
```

but the same layout regrew to:

```text
1.0 mm imbalance = 2.625
```

A more transverse-heavy static partition:

```text
simple 4x1x12
```

reduced 0.5 mm imbalance to 1.401, but increased wall/step by ~1.5% and total
ClockTime by 33.1%.

Conclusion: a static decomposition can move the compromise between cell-count
balance and processor-interface/communication cost, but the moving refinement
band continues to evolve away from any fixed initial partition.

## Next architecture

Use a fast static initial layout, then rebalance an already refined checkpoint.

OpenFOAM `redistributePar` is designed to redistribute an existing decomposed
mesh and fields according to the current `decomposeParDict`. For this project
the proposed first target is:

```text
initial decomposition: simple 8x1x6
checkpoint:            normal write time
redistribution method: scotch on the current refined mesh
destination ranks:     48
resume:                latest redistributed checkpoint
```

## Why Scotch is different here

The original long-track Scotch case decomposed the *initial* nearly uniform
base mesh. The refined region subsequently moved and accumulated unevenly.

Checkpoint redistribution would instead run Scotch on the *current refined
mesh*. Thus the graph being partitioned already contains the moving-AMR
workload accumulated up to the checkpoint.

This is a different operation from simply choosing Scotch at t=0.

## Required smoke-test gates

Do not use redistribution on a production 3 mm campaign until all of the
following pass:

1. Run a short case to a normal write time.
2. Preserve a copy of the checkpoint before redistribution.
3. Redistribute the existing decomposed mesh and fields on all 48 ranks.
4. Verify cell totals and improved rank balance.
5. Restart electronBeamFoam from the redistributed latest time.
6. Confirm dynamicRefineFvMesh can refine/unrefine after restart.
7. Confirm cumulative `everMelted` / `peakTemperature` diagnostics continue
   rather than reset.
8. Compare final W/D/L, power conservation and continuity against an otherwise
   identical non-redistributed reference.
9. Include redistribution overhead in performance accounting.

Only after restart fidelity is established should the workflow move to a 1 mm
performance test.

## Decision criterion

A redistribution workflow is useful only if:

```text
saved solver wall time
>
redistribution + checkpoint overhead
```

and the physical/numerical outputs remain inside the accepted envelope.

The target is campaign throughput, not the smallest possible cell-count
imbalance.


## Implemented smoke workflow

The first checkpoint-redistribution fidelity gate is now automated in:

```text
tutorials/electronBeamFoam/barePlate_calibration_Zakirov2020/
    Run_redistributionSmoke
    Allrun_redistributionSmoke
    CompareRedistributionSmoke.py
    system/decomposeParDict.redistributeScotch
```

The workflow runs the accepted 0.5 mm trajectory in two solver stages around a
midpoint redistribution. It backs up all processor directories before
redistribution, audits cumulative fields and hexRef8 data before and after,
runs `checkMesh -parallel -latestTime`, requires post-restart AMR activity,
and compares final fusion geometry against the accepted unsegmented 0.5 mm
simpleXZ baseline.

This smoke is deliberately not a performance benchmark. The tar backup,
checkMesh and process restart are included for safety and fidelity evidence.
