# electronBeamFoam calibration status — 2026-09-23

## Current objective

Establish a numerically stable, computationally affordable bare-plate EPBF
baseline before quantitative comparison with the Zakirov2020 Ti-6Al-4V
bead-on-plate dataset and subsequent source-model calibration.

## Completed numerical gates

1. **Electron-beam deposition / tracking**
   - multiRayFirstHit with 60 beamlets;
   - all beamlets hit the metal in current calibration preflights;
   - integrated deposited power matches requested absorbed power to numerical
     roundoff.

2. **Phase-change convergence**
   - consistent metal phase-change properties enabled in VOF interface cells;
   - metal-weighted phase residual used for convergence;
   - epsilonTolerance 1e-4 -> 1e-3 accepted;
   - W/D/L unchanged; fusion volume +0.184%;
   - ClockTime 19112 -> 14091 s (1.356x).

3. **Calibration mesh**
   - abrupt three-block gradedY rejected after a parallel AMR topology-update
     stall;
   - single-block gradedYSmooth accepted;
   - base cells 1,013,760 -> 696,960 (-31.25%);
   - final dynamic cells 2,241,196 -> 1,923,255 (-14.19%);
   - W/D/L unchanged; fusion volume -0.097%;
   - ClockTime 14091 -> 9626 s (1.464x).

4. **Pressure linear solver**
   - GAMG/DICGaussSeidel rejected: pressure wall +13.8%, ClockTime +7.1%;
   - PCG/FDIC rejected: pressure wall +3.6%, ClockTime +2.7%;
   - PCG/DIC remains the accepted linear-solver baseline.

5. **PIMPLE pressure corrections**
   - nCorrectors 3 -> 2 accepted;
   - W/D/L unchanged; fusion volume -0.151%;
   - pressure wall 2946.9 -> 2351.8 s (1.253x);
   - ClockTime 9626 -> 8926 s (1.078x);
   - continuity gate passed: max |global|=6.07e-10, final cumulative=4.19e-9.

Overall, relative to the first 900 W / 3 m/s / 0.5 mm preflight, the current
two-corrector candidate reduces ClockTime from about 19112 s to 8926 s
(2.14x faster, about 53% lower wall time).

## What the 0.5 mm preflight does and does not prove

The short preflight is a numerical/performance gate. Its global cumulative
fusion-zone width/depth must **not** be interpreted as the final experimental
calibration error because:

- the simulated track is only 0.5 mm;
- the reference numerical track is 3 mm;
- the current fusionZone W/D are global bounding-box extrema over the entire
  melted history, whereas metallographic measurements are cross-sectional.

The current short-track W=435 um and D=61 um therefore remain diagnostic values,
not final fitted predictions.

## Remaining gates to the first quantitative target

### A. Freeze the numerical baseline — complete
The accepted baseline is gradedYSmooth + epsilonTolerance=1e-3 + PCG/DIC +
nCorrectors=2 on 48 physical cores. Pressure micro-optimization is closed.

### B. Make the experimental observable correct
Implement station-wise fusion-zone cross-section diagnostics:
- user-selected x stations or a central-track window;
- width and depth measured in each y-z section;
- exclude start/stop transients from the primary comparison;
- retain global fusion-zone diagnostics as a secondary quantity.

This is required before treating W/D as quantitative metallographic outputs.

### C. Run the first full 3 mm reference
296 K / 900 W / 3 m/s with the frozen numerical baseline. A purely linear
runtime extrapolation from the current 0.5 mm candidate is about 14.9 h on
48 physical cores, but the actual full-track cost must be measured.

### D. Validate/calibrate across the Zakirov family
Use multiple conditions rather than fitting one W/D pair:
- 296 K: 1000, 3000, 6000 mm/s;
- 973 K: 1000, 3000, 6000 mm/s.

Constrain absorptivity, beam radius and penetration depth from trends; do not
fit all three to one condition.

### E. Publication-quality numerical evidence
- near-surface/dynamic-refinement sensitivity study;
- station-location sensitivity for cross-sectional W/D;
- timestep/Courant sensitivity on at least one representative condition;
- quantify run-to-run performance variability separately from physics changes.

### F. Physics fidelity after the Gaussian baseline
- revisit substrate thermal boundary conditions, especially for 973 K preheat;
- validate evaporation/recoil/Marangoni contributions with controlled switches;
- introduce acceleration-voltage-dependent penetration/spot-size data;
- progress from exponential volumetric Gaussian deposition to tabulated
  Monte-Carlo electron deposition when the baseline is validated.

The immediate critical path is:
**station-wise W/D diagnostic smoke test -> full 3 mm 296 K baseline**.

The station-wise diagnostic is now implemented as a finite-slab measurement at
-0.5/0/+0.5 mm relative to the track reference point. It must be compiled and
smoke-tested before starting the full reference run.


## Section-diagnostic integration result

The 0.05 mm / 16.7 us smoke test completed successfully with the
`fusionSections-v4-pCorr2Baseline` build and wrote all configured section
rows. No material melted during this deliberately short exposure, so all
section W/D values were zero. This passes runtime/CSV integration but not the
positive-signal geometry gate.

Next gate: `Run_sectionSignalProbe` at 0.25 mm / 83.33 us. The central x=0
section should become non-zero; +/-0.5 mm stations remain outside the short
track by design.
