# First completed 3 mm Zakirov2020 reference

Condition: Ti-6Al-4V bare plate, 296 K, 900 W, 3 m/s, 3 mm scan.

## Primary quantitative observable

The accepted metallographic observable is the mean of the final cumulative
fusion-zone sections at x = -0.5, 0, +0.5 mm:

| station (mm) | W (um) | D (um) |
|---:|---:|---:|
| -0.5 | 441.437 | 67.250 |
| 0.0 | 435.187 | 67.250 |
| +0.5 | 441.437 | 67.250 |

Mean: W=439.354 um, D=67.250 um.

Against the stored Zakirov target W=525 um, D=51 um:
- width error = -16.31%;
- depth error = +31.86%;
- station spread = 6.25 um in width and 0 um in depth.

The small central-window spread indicates that the three central sections are
sampling a developed track region. The whole-track bounding box remains
secondary (W=447.687 um, D=67.250 um).

## Numerical health

The run reached End. Beam tracking remained 60/60 with deposited power equal
to the requested 765 W to roundoff. Continuity remained small:
max |global|=8.23e-10 and final cumulative=5.08e-8.

The main unresolved numerical issues are:

1. **Parallel load imbalance.** The final mesh had 2,896,318 cells, but local
   rank counts ranged from 14,375 to 237,982; max/mean imbalance=3.944.
   Long-track performance is therefore dominated by poor static partition
   balance after dynamic refinement.
2. **Late interface thermal convergence.** Thermal cap hits became frequent in
   some developed intervals; the reported cap regions remained at the
   interface, while bulk residuals stayed below the accepted tolerance.
3. **Depth quantisation.** The reported 67.25 um depth is on the current
   refined-grid resolution scale; a near-surface/depth mesh sensitivity check
   is required before fitting penetration physics tightly.

## Physical interpretation for the next calibration stage

The present Gaussian source produces a fusion zone that is too narrow and too
deep relative to experiment. That directional mismatch should not be corrected
by fitting absorptivity, radius and penetration simultaneously to this single
condition.

Before the six-condition campaign:
- fix/de-risk MPI decomposition/load balance;
- perform a small source-shape sensitivity matrix, prioritising beam radius
  and penetration depth while holding absorptivity fixed;
- perform at least one local mesh-sensitivity check for depth.

The completed 3 mm case is the immutable baseline for those comparisons.
