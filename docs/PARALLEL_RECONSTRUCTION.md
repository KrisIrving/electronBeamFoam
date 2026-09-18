# Parallel reconstruction for dynamicRefineFvMesh

The bare-plate tutorials use `dynamicRefineFvMesh`. During a parallel run,
topology changes create new processor-local mesh instances such as:

```
processor0/2e-05/polyMesh/
```

The original `decomposePar` addressing exists for the original mesh only.
Therefore running `reconstructPar` directly can fail with an error such as:

```
cannot find .../processor0/2e-05/polyMesh/pointProcAddressing
```

For a topology-changing mesh, reconstruct the changed mesh instances first.
The tutorial `Reconstruct` script does this automatically:

1. discovers every processor time containing `polyMesh/points`;
2. runs `reconstructParMesh -time <time>` for each changed mesh instance;
3. runs `reconstructPar` for the fields;
4. creates `case.foam` for ParaView.

Normal parallel runs now call this script automatically after the solver exits
successfully. Disable this behaviour for a large production run with:

```bash
AUTO_RECONSTRUCT=0 NPROCS=32 ./Run_background
```

To reconstruct an existing completed decomposed case manually:

```bash
./Reconstruct
```

The processor directories are deliberately retained after reconstruction.


## Validation status

Validated on 2026-09-18 with the completed parallel
`barePlate_singleTrack` dynamic-refinement result. The `Reconstruct` script
completed without error, successfully rebuilding the topology-changing mesh,
reconstructing the fields and creating `case.foam` for ParaView.
