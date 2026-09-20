# Phase-change convergence diagnostics

The short 48-rank moving-track regression showed that the legacy
laserbeamFoam liquid-fraction corrector reached its configured correction cap
on nearly every CFD step. The global final liquid-fraction residual also
remained O(1). Before changing tolerances or the phase-change formulation,
electronBeamFoam now reports where that residual occurs.

At each completed temperature/phase solve, the final epsilon1 correction is
partitioned into:

- bulk metal: alpha.metal >= 0.99;
- VOF interface: 0.01 < alpha.metal < 0.99;
- numerical void: alpha.metal <= 0.01.

Write-time performance summaries report the average of each region's final
maximum residual, the largest observed regional residual, and how many capped
solves had each region above epsilonTolerance.

This is diagnostic-only. No convergence criterion, relaxation factor, energy
equation, liquid-fraction update, or physical model is changed.

Use a short regression first. The result determines whether the next change
should target the bulk-metal enthalpy iteration, the metal/void transition
treatment, or the numerical-void phase.
