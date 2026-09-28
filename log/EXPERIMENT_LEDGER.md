# Experiment and numerical-test ledger

This ledger is intentionally compact. Detailed accepted values live in
`validation/` and detailed technical descriptions in `docs/`.

| Test | Controlled change | Track | Main result | Decision |
|---|---|---:|---|---|
| Early 900 W phase-change baseline | strict raw/legacy convergence | short | excessive thermal corrections/caps | superseded |
| Metal-consistent phase change | interface material treatment | short | residual behaviour improved | ACCEPT |
| epsilon A/B | 1e-4 -> 1e-3 | 0.5 mm | W/D/L unchanged; runtime improved | ACCEPT 1e-3 |
| Mesh A/B | uniformFine -> abrupt gradedY | 0.5 mm | AMR topology-update stall | REJECT gradedY |
| Mesh A/B | uniformFine -> gradedYSmooth | 0.5 mm | W/D/L unchanged; fewer cells; faster | ACCEPT gradedYSmooth |
| Pressure A/B | PCG/DIC -> GAMG | 0.5 mm | slower | REJECT GAMG |
| Pressure A/B | DIC -> FDIC | 0.5 mm | slightly slower | REJECT FDIC |
| PIMPLE A/B | nCorrectors 3 -> 2 | 0.5 mm | W/D/L unchanged; continuity passed; faster | ACCEPT 2 |
| Section smoke | station diagnostic integration | 0.05 mm | CSV/runtime path works; no melt | PASS integration only |
| Section positive signal | non-zero section geometry | 0.25 mm | central section non-zero and consistent | ACCEPT diagnostic |
| Full Zakirov reference | first quantitative full track | 3 mm | mean W=439.354 um, D=67.250 um | ACCEPT baseline |
| Full-reference recovery | checkpoint resume after MPI peer reset | 0.3 -> 1.0 ms | completed without restart from zero | ACCEPT recovery workflow |
| Long-track MPI diagnosis | dynamic AMR + Scotch | 3 mm | final max/mean cell imbalance 3.944 | PERFORMANCE ISSUE |
| Decomposition A/B | Scotch -> simpleXZ (8 1 6) | 0.5 mm | W/D/L unchanged; V -0.237%; ClockTime 2.095x faster; imbalance 1.683 | PASS -> 1 mm |
| Decomposition confirmation | simpleXZ (8 1 6) | 1.0 mm | x=0 W/D exactly matches 3 mm baseline; imbalance grows to 2.625 | DO NOT PROMOTE; test 4x1x12 |

## Evidence paths

Accepted 0.5 mm numerical baseline:

```text
validation/Zakirov2020_preflight_accepted_baseline.json
```

First full 3 mm result:

```text
validation/Zakirov2020_full_296K_3000mmps_baseline.json
docs/FULL_REFERENCE_296K_3000_RESULTS.md
```

Current decomposition A/B:

```text
tutorials/electronBeamFoam/barePlate_calibration_Zakirov2020/
    Run_decompSimpleXZProbe
    CompareDecomposition.py
```

When reviewed, commit the probe result to `validation/` and update this row
rather than replacing the historical entry.
