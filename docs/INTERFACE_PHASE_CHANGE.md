# Interface-consistent metal phase change

## Diagnostic evidence

The region-resolved 25 us moving-track test localized the phase-change
non-convergence to the VOF metal/void interface:

- bulk-metal final residual: meanMax about 1.38e-4, max about 7.4e-3;
- interface final residual: meanMax about 8.36e-2, max = 1;
- numerical-void residual: zero;
- the O(1) global residual therefore came from mixed VOF cells.

## Cause in the legacy formulation

The legacy laserbeamFoam implementation interpolates Cp and latent heat across
the dense/gas VOF fraction and then updates the liquid fraction with a factor
proportional to Cp/Lf. In the interface, Lf decreases strongly with metal
fraction while the cell still contains finite heat capacity. The correction
gain therefore becomes alpha-dependent and can become large enough to clip the
liquid fraction between zero and one on successive nonlinear corrections.

The same interpolation also makes the latent enthalpy coefficient scale with
alpha even though the mixture density already carries the leading dense-phase
weighting, giving an approximate alpha-squared suppression of latent heat in
mixed cells.

## New optional treatment

`consistentMetalPhaseChange` changes only the metal-containing VOF cells:

- use the metal solidus/liquidus when alpha.metal exceeds the cutoff;
- use metal Cp and metal Lf in the liquid-fraction correction;
- use metal Lf in the latent-heat transport coefficient.

Near-pure numerical void remains on the legacy gas branch. This preserves
epsilon1 approximately equal to one in the void and avoids introducing Darcy
damping there.

The option is enabled first only for `barePlate_movingSingleTrack`. It must
pass the short A/B regression before becoming the default for other tutorials
or before experimental calibration.
