# Decision register

Status values:
- **ACCEPTED** — current baseline unless explicitly reopened;
- **REJECTED** — tested and not used;
- **DEFERRED** — architecturally planned, evidence not yet sufficient;
- **ACTIVE** — current gate.

| Decision | Status | Evidence / rationale |
|---|---|---|
| Numerical void for chamber secondary phase | ACCEPTED | Current solver does not claim a rarefied-gas chamber solution. |
| Volumetric Gaussian electron deposition | ACCEPTED as baseline | Stable engineering source for bare-plate calibration; not final electron-transport model. |
| `multiRayFirstHit` surface mapping | ACCEPTED | Handles local moving free-surface hit position; full-track runs retain 60/60 hits. |
| multiRay cache | REJECTED as default | Prior timing showed no useful total-runtime gain; reference remains fully retraced. |
| `epsilonTolerance=1e-3` | ACCEPTED | Geometry preserved versus 1e-4 with meaningful runtime reduction. |
| Abrupt multi-block `gradedY` | REJECTED | Parallel dynamic-mesh topology-update stall. |
| Single-block `gradedYSmooth` | ACCEPTED | Geometry preserved, fewer cells and lower runtime. |
| GAMG pressure profile | REJECTED | Slower than PCG/DIC in controlled calibration probe. |
| FDIC preconditioner | REJECTED | Slightly slower than DIC. |
| PIMPLE `nCorrectors=2` | ACCEPTED | Geometry preserved, continuity passed, runtime improved. |
| Whole-track bounding-box W/D as experimental observable | REJECTED | Does not correspond to metallographic cross section. |
| Central -0.5/0/+0.5 mm station mean W/D | ACCEPTED | Small station spread in completed 3 mm reference. |
| Restart full failed track from zero | REJECTED policy | Preserve expensive valid common checkpoints and resume. |
| Scotch decomposition for long calibration campaign | UNDER REVIEW | Completed run but dynamic-AMR imbalance reached ~3.94 max/mean. |
| `simpleXZ (8 1 6)` decomposition | REJECTED as final campaign default | Physics passed and x=0 W/D matched the 3 mm baseline, but imbalance regrew 1.683 -> 2.625 by 1 mm. Retained as benchmark. |
| `simpleXZ (4 1 12)` decomposition | ACTIVE | Next 0.5 mm static-layout gate; hypothesis is to distribute the narrow refined track across more z partitions. |
| Fit eta/rb/penetration simultaneously to one W/D pair | REJECTED methodology | Non-identifiable / confounds source effects. |
| Tabulated Monte-Carlo deposition | DEFERRED | Architecture reserved; table reader/physics database not implemented yet. |
