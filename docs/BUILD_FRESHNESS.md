# Build freshness guard

Long EPBF runs must not start with an executable compiled before the latest
solver/source changes. Each tutorial runner now compares the
`electronBeamFoam` executable modification time with C++ sources under:

```text
applications/solvers/electronBeamFoam
src/electronBeamHeatSource
```

If a newer source file exists, the run exits before meshing/decomposition and
asks for `./Allwmake`.

The solver also prints:

```text
electronBeamFoam build tag = phaseResidualRegions-v1
```

and `SummarizeRun` preserves this line. This makes it obvious whether a run
contains the region-resolved phase-change convergence diagnostics.
