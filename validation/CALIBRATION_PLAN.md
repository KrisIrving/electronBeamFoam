# Calibration sequence

The first quantitative bare-plate validation family is based on
Zakirov et al. (Additive Manufacturing 35, 101236).

Recommended sequence after the phase-change convergence gate:

1. 296 K, 900 W, 3000 mm/s:
   moderate line energy (0.3 J/mm) and measured W=525 um, D=51 um.
2. 973 K, 900 W, 3000 mm/s:
   same beam input with preheating; measured W=573 um, D=77 um.
3. 296 K speed sweep:
   1000 / 3000 / 6000 mm/s.
4. 973 K speed sweep.
5. Only after the model reproduces the trends, use the Li et al. 60/90 kV
   dataset to test acceleration-voltage-dependent penetration and spot size.

Initial source values for stage 1:
- absorptivity = 0.85;
- beamRadius = 250e-6 m (nominal D4sigma=500 um);
- penetrationDepth = 18.9e-6 to 20e-6 m.

Do not fit all three source parameters simultaneously to a single width/depth
pair. Use the multi-condition dataset to constrain them.


Implemented reference tutorial:

```text
tutorials/electronBeamFoam/barePlate_calibration_Zakirov2020
```

The default run is the 296 K / 900 W / 3000 mm/s / 3 mm-track baseline.
Use `Run_preflight` only as a shortened numerical smoke test; quantitative
experimental comparison should use the full default track.


Numerical calibration baseline update:
- the 900 W preflight A/B accepted `epsilonTolerance=1e-3`;
- W/D/L were unchanged at the reported mesh resolution;
- fusion-zone volume shifted by +0.184%;
- wall-clock speedup was 1.356x;
- the next gate is the uniformFine vs gradedY mesh A/B.


Mesh baseline update:
- `gradedYSmooth` accepted as the calibration default;
- W/D/L unchanged versus uniformFine at reported resolution;
- fusion-zone volume -0.097%;
- final dynamic cell count -14.19%;
- ClockTime 14091 -> 9626 s (1.464x);
- next performance gate: pressure linear-solver A/B on the same 0.5 mm case.


Pressure-solver update:
- GAMG/DICGaussSeidel rejected on the 0.5 mm gradedYSmooth case;
- W/D/L unchanged, but pressure wall time increased ~13.8%;
- total ClockTime increased ~7.1%;
- PCG/DIC remains baseline;
- next gate is PCG/FDIC with identical tolerances.


Pressure-preconditioner update:
- PCG/FDIC rejected;
- geometry and dynamic-cell path were effectively identical to PCG/DIC;
- pressure wall time increased ~3.6%, total ClockTime ~2.7%;
- next gate: reduce PIMPLE pressure correctors from 3 to 2 while retaining
  the strict final p_rgh solve.


Two-corrector candidate:
- nCorrectors 3 -> 2 preserved W/D/L; fusion volume -0.151%;
- pressure wall speedup 1.253x; total ClockTime 9626 -> 8926 s (1.078x);
- final dynamic cells +0.03%;
- continuity gate passed: max |global|=6.07e-10, final cumulative=4.19e-9;
- nCorrectors=2 accepted as the calibration default;
- pressure micro-optimization is closed;
- next: station-wise metallographic fusion-zone cross sections.


Section-diagnostic gate:
- 0.05 mm smoke test passed runtime/CSV integration but remained below melt;
- 0.25 mm positive-signal probe produced a 1077-cell central section;
- central section W/D exactly matched the simultaneous global W/D in the
  symmetric short-track state (372.687 / 42.250 um);
- +/-0.5 mm stations remained zero as expected outside the short scan;
- continuity max |global| = 3.85e-10;
- station-wise diagnostic accepted for the first full 3 mm quantitative run.


First completed 3 mm quantitative reference:
- 296 K / 900 W / 3000 mm/s completed;
- primary central-window mean: W=439.354 um, D=67.250 um;
- experimental errors: W -16.31%, D +31.86%;
- central station spread: dW=6.25 um, dD=0;
- beam power conservation and continuity passed;
- parallel cell imbalance reached 3.94 max/mean;
- next gate is performance-safe load balancing plus source-shape/mesh
  sensitivities before launching the full six-condition campaign.


Immediate next gate after the completed 3 mm baseline:
1. 0.5 mm decomposition A/B: current scotch baseline vs simpleXZ (8 x 1 x 6);
2. if promising, confirm on a 1 mm moving-track run;
3. only then promote a decomposition strategy for the six-condition campaign;
4. follow with depth mesh sensitivity and a small beam-radius/penetration
   source-shape sensitivity matrix.


0.5 mm simpleXZ gate passed:
- W/D/L unchanged versus accepted Scotch baseline;
- fusion volume -0.237%;
- ClockTime 8926 -> 4260 s (2.095x);
- final-interval wall 6535 -> 2979 s (2.194x);
- final simpleXZ imbalance max/mean=1.683;
- initial base decomposition was exactly 14,520 cells/rank;
- final interval used 25.6% fewer time steps, but step-normalized wall cost
  still improved ~1.63x;
- next gate: 1 mm simpleXZ confirmation before campaign promotion.


1 mm simpleXZ (8 1 6) confirmation:
- solver reached End;
- central x=0 W/D = 435.187 / 67.250 um, identical to completed 3 mm baseline;
- continuity and beam power accounting remained acceptable;
- max/mean cell imbalance grew 1.683 -> 2.625 from 0.5 to 1.0 mm;
- 8x1x6 is therefore not promoted as the final campaign decomposition;
- next static-layout gate: simple (4 1 12) on 0.5 mm;
- if static layouts still show strong track-length growth, move to checkpoint
  redistribution / dynamic load balancing instead of further static tuning.


4x1x12 static-layout gate:
- W/D/L unchanged; fusion volume +0.204%;
- imbalance improved 1.683 -> 1.401;
- final wall/step worsened ~1.5%;
- final-interval wall worsened ~24%;
- ClockTime worsened 4260 -> 5672 s (+33%);
- do not advance 4x1x12 to 1 mm;
- stop static-layout search for now;
- next gate: checkpoint redistribution / dynamic rebalancing using
  redistributePar on a controlled 1 mm track.


0.5 mm simpleXZ (4 1 12) gate:
- W/D/L unchanged; fusion volume +0.204%;
- max/mean cell imbalance improved 1.683 -> 1.401;
- final-interval steps increased 22.2%;
- wall/step worsened ~1.5%;
- ClockTime worsened 4260 -> 5672 s (+33.1%);
- candidate rejected and not advanced to 1 mm.

Static decomposition search is paused. The next performance gate is a short
checkpoint-redistribution workflow using OpenFOAM redistributePar on the
existing decomposed dynamic mesh and fields. The smoke test must verify
dynamic-refinement restart state, cumulative fields and physical invariance
before a 1 mm performance comparison.


Checkpoint-redistribution fidelity smoke:
- implemented as a two-stage 0.5 mm run around the normal midpoint write;
- starts from simpleXZ 8x1x6 and redistributes the refined checkpoint to
  48-rank Scotch;
- uses redistributePar -latestTime -overwrite;
- audits everMelted, peakTemperature and hexRef8 refinement files before/after;
- preserves a full processor-directory rollback archive;
- requires post-redistribution checkMesh and new AMR activity after restart;
- final W/D/L are compared to the accepted unsegmented 0.5 mm baseline.

If fidelity passes, the next gate is a 1 mm performance A/B with one controlled
redistribution checkpoint.


Redistribution-smoke first local execution observation:
- the 0.5 mm workflow completed Stage 1 to the exact midpoint checkpoint
  (8.33333e-05 s) with 60/60 beamlets hitting metal and power error
  approximately 2.3e-13 W;
- midpoint simpleXZ load imbalance had already grown to max/mean=1.304,
  confirming that a mid-track rebalance remains worth testing;
- the wrapper was no longer active and Stage 2 had not started, so this is
  recorded as a workflow interruption, not as a redistribution-fidelity fail;
- persistent workflow-stage/failure markers and Status reporting were added
  before the next rerun so the exact failing operation can be identified
  without manually inspecting many logs.


Redistribution-smoke interruption root cause and workflow fix:
- the first local smoke run did not reach redistributePar; Stage 1 itself
  completed normally at the expected first normal write, 8.33333e-05 s;
- the workflow derived its checkpoint from half of the rounded controlDict
  endTime (0.000166667 / 2 = 8.33335e-05 s), while the independently rounded
  normal writeInterval produced the actual 8.33333e-05 s checkpoint;
- the exact-checkpoint guard therefore stopped the workflow before the
  pre-redistribution file audit and before redistributePar;
- the smoke workflow now uses the generated normal writeInterval directly and
  allows only a small representation-level time-name tolerance;
- Status duplicate-tail syntax damage and the smoke-state worker PID marker
  were also corrected;
- this event is a harness bug only and provides no evidence for or against
  redistribution fidelity.


Checkpoint-redistribution fidelity smoke result:
- PASS on the controlled 0.5 mm case;
- final W/D/L exactly reproduced the accepted uninterrupted simpleXZ
  baseline, while fusion-zone volume changed by +0.357%;
- everMelted, peakTemperature and all audited hexRef8 refinement-state files
  survived redistribution on all 48 ranks;
- parallel checkMesh passed and the resumed solver subsequently recorded
  452 refine and 452 unrefine events;
- midpoint max/mean imbalance 1.304 was reset to 1.010 by Scotch, then regrew
  to 1.541 by the final write;
- the smoke workflow took 6053 s versus about 4260 s for the accepted
  uninterrupted 0.5 mm simpleXZ run, so improved cell balance alone is not
  evidence of a performance gain;
- next gate: controlled 1 mm performance comparison.  Preserve the accepted
  1e-4 s write cadence, redistribute at the second normal write (2e-4 s),
  and compare the final 3e-4 s fusion-zone output plus full workflow wall time
  against validation/Zakirov2020_decomp_simpleXZ_1mm.json (24059 s baseline).


1 mm checkpoint-redistribution performance gate:
- candidate used the accepted 1 mm simpleXZ trajectory and write cadence,
  with a single Scotch redistribution at t=2e-4 s;
- pre-redistribution max/mean imbalance was 2.311 and Scotch reduced it to
  1.010 immediately; final imbalance regrew only to 1.753;
- at t=3e-4 s W and D were unchanged and fusion volume differed by +0.125%;
- fusion length differed by +0.573%, narrowly exceeding the predeclared 0.5%
  fidelity threshold, so the formal fidelity result remains REVIEW rather than
  retroactively relaxing the gate;
- the performance result is unambiguous: full workflow wall time was 31893 s
  versus the accepted uninterrupted simpleXZ baseline of 24059 s, i.e.
  0.754x speed / about 32.6% slower;
- redistributePar itself cost only 16 s and checkMesh 1 s, so the regression
  is dominated by the post-redistribution solver path rather than the
  redistribution operation;
- checkpoint redistribution is therefore REJECTED for the current validation
  campaign.  Keep simpleXZ as the engineering baseline and close static/dynamic
  decomposition tuning for now;
- future performance work must consider communication topology and processor
  interfaces, not cell-count balance alone.

Performance optimisation branch closed.  The next development stage is
Vacuum Fidelity Verification: quantify implied evaporative mass loss first,
then test numerical-void thermophysical-property sensitivity before resuming
source-shape calibration and the multi-condition experimental campaign.


Vacuum Fidelity Verification opened:
- formal test plan: docs/VACUUM_FIDELITY_PLAN.md;
- V0 adds diagnostic-only conversion of the existing evaporation latent-heat
  sink into implied mass rate and cumulative evaporated mass; no metal mass is
  removed from alpha.metal yet;
- cumulativeEvaporatedMassDensity is restart/AMR aware when diagnostics are
  enabled in EPBFPhysicsProperties;
- first execution gate is the accepted 0.5 mm simpleXZ path via
  `./Run_preflight vacuum`;
- use the V0 result to decide whether explicit evaporative mass transfer is
  physically required before proceeding to numerical-void k/rho/nu
  sensitivities.


Status ETA monitoring update:
- removed the hard-coded 1.0 ms ETA target;
- generateCalibrationTrack now records cool time and the exact generated
  physical end time in log.generateTrack;
- Status resolves the target from the generated case metadata, so 0.5 mm,
  1 mm, 3 mm and arbitrary scan-speed cases are handled automatically;
- current ETA uses a recent ~15 minute wall-clock throughput window rather
  than a fixed number of CFD steps;
- concatenated restart logs are split when OpenFOAM ExecutionTime/ClockTime
  resets, preventing invalid cross-restart throughput estimates;
- Status now reports physical progress, current local pace, current solver
  segment average, ETA to the case end and projected local finish time; a
  second segment-average ETA is shown when the workload is changing by enough
  to move the estimate by at least 10%.


Vacuum Fidelity V0 live-monitoring refinement:
- the first diagnostic run confirmed the adaptive ETA correctly recognizes the
  0.5 mm / 166.667 us physical endpoint;
- Status now prints the write-time timestamp for evaporation, melt-pool and
  fusion-zone data so live solver progress cannot be confused with stale
  write-time physics;
- SummarizeVacuumFidelityV0.py compares implied evaporated volume with
  fusion-zone volume and evaporation power with absorbed beam power at the
  latest common write time;
- Allrun_parallel archives the completed summary as vacuum-fidelity-v0.txt.


Vacuum Fidelity V0 final decision:
- completed 0.5 mm simpleXZ run reached the generated physical endpoint;
- cumulative implied evaporated mass = 2.469e-11 kg;
- equivalent evaporated volume = 5.573e-15 m3, about 0.096% of the
  5.812e-12 m3 cumulative fusion-zone volume;
- final evaporation power = 2.953 W, about 0.386% of 765 W absorbed beam power;
- peak local recession speed = 3.401e-3 m/s; even the deliberately
  conservative full-scan peak-speed upper bound is about 0.57 um, below the
  nominal ~1.56 um finest interface y scale from 6.25 um base spacing and
  maxRefinement=2;
- explicit evaporative mass removal is therefore deferred for the present
  calibration regime. Revisit this gate if later conditions produce
  substantially stronger evaporation.

Vacuum Fidelity V1 opened:
- vary only gas/numerical-void thermal conductivity by 1x / 0.1x / 0.01x;
- exact completed V0 outputs are archived as the 1x local baseline before
  candidates are run;
- primary gate: |dW|, |dD|, |dL| <= 0.5%, |dV| <= 1%, and temperature
  changes <= 1%;
- secondary evaporation gate uses absolute importance rather than only
  relative change: evap/fusion volume <=0.5% and evaporation power <=1% of
  absorbed beam power;
- execute with `./Run_preflight kappa`; compare exact archived summaries in
  `vacuumFidelity/V1_kappa/comparison.txt`.


Vacuum Fidelity V1 result:
- completed 1x / 0.1x / 0.01x numerical-void conductivity sweep;
- W/D/L were unchanged and fusion volume remained within +0.26%;
- 0.1x passed the predeclared primary and absolute-evaporation gates;
- 0.01x preserved geometry but fusion peak temperature changed +1.153%,
  exceeding the 1% gate; recoil changed +15.99% and final evaporation power
  changed +3.905%;
- absolute evaporation remained small at about 0.096% of fusion volume and
  about 0.4% of absorbed beam power;
- V1 is therefore REVIEW rather than PASS. Do not advance to rho/nu
  sensitivity yet.

Vacuum Fidelity V1b opened:
- extend k_void from the existing 0.01x case to 0.001x and 0.0001x;
- assess the last decade (0.001x -> 0.0001x) for a low-conductivity plateau;
- predeclared plateau gate: W/D/L <=0.25%, V <=0.5%, Tmax <=0.5%,
  recoil <=5%, Qevap <=2%, while absolute evaporation remains <=0.5%
  fusion volume and <=1% absorbed power;
- if the plateau passes, select the largest conductivity inside it (0.001x)
  for numerical robustness and then proceed to V2;
- execute with `./Run_preflight kappa-limit`.


Workflow-launch diagnostics improvement:
- Run_preflight background validation modes now verify the worker one second
  after launch;
- a successful launch prints STARTED plus the current state marker and the
  monitoring command;
- an immediately failing worker prints its failure marker and the tail of the
  workflow log directly to the terminal instead of silently leaving Status on
  the previous solver log;
- Status now surfaces the current V1/V1b validation workflow state before the
  calibration/solver diagnostics.


V1b preflight robustness fix:
- a local V1b launch exited at preflight before k0p001 started;
- the only V1b prerequisite at that point was the archived k0p01 summary;
- V1b now reconstructs the missing k0p01 archive automatically when the
  current working case can be verified as a completed 0.01x run (log scale,
  completed solver log and required diagnostics all present);
- otherwise the preflight error now prints the expected archive path and the
  detected current void-k scale;
- Run_preflight also reports the exact workflow-log path and explicitly says
  when an immediate-failure log exists but is empty.


V1b comparison/resume fix:
- both low-k CFD candidates completed successfully; the observed failure was
  confined to the final comparison stage;
- CompareVacuumKappaConvergence.py was rewritten with explicit schema/finite
  checks and traceback reporting;
- convergence stderr is now preserved separately;
- Allrun_vacuumKappaConvergence is resume-safe: existing k0p001/k0p0001
  summaries are reused and only missing candidates are recalculated.


Vacuum Fidelity V1b final:
- workflow completed and both low-k candidates were archived;
- final decade 0.001x -> 0.0001x: W/D/L 0%, V +0.081%, fusion Tmax
  +0.478%, melt Tmax +0.628%, recoil +11.535%, Qevap +0.573%;
- V1b is REVIEW: the predeclared melt-Tmax and recoil plateau gates fail;
- do not continue reducing cell-centred k_void and do not advance to V2 yet.

Vacuum Fidelity V1c opened:
- hypothesis: the remaining artificial vacuum heat path is controlled by the
  face interpolation used by fvm::laplacian(kappa,T), not simply the pure-void
  cell value;
- Allrun_parallel now supports THERMAL_KAPPA_INTERP=linear|harmonic and restores
  fvSchemes after every run;
- V1c uses OpenFOAM harmonic interpolation for the thermal diffusion
  coefficient at 1x / 0.1x / 0.01x cell k_void;
- execute with `./Run_preflight thermal-face`;
- compare the harmonic 0.1x -> 0.01x decade against the same plateau gate
  before considering V2.
