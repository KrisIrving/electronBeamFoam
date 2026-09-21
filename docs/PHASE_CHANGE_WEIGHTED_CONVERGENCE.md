# Metal-weighted phase-change convergence

The interface-consistent phase-change correction reduced the 25 us test's
interface residual from O(1) to O(1e-3) without materially changing the
fusion-zone dimensions. However, the legacy stopping rule still used the raw
maximum liquid-fraction change with epsilonTolerance=1e-6, causing nearly every
solve to reach maxTempCorrector.

For a mixed VOF cell, epsilon1 represents the liquid fraction of the metal
carried by that cell. The latent-energy effect of a correction therefore scales
with the local metal fraction. The optional `metalWeightedMax` criterion uses:

```text
max(alpha.metal * abs(Delta epsilon1))
```

as the nonlinear convergence residual.

This leaves bulk-metal cells unchanged because alpha.metal approaches one.
Near the metal/void interface, the tolerance is applied to the metal-weighted
liquid-fraction correction rather than to a numerically amplified raw change
in a cell containing only a fraction of metal.

The moving calibration tutorial initially uses epsilonTolerance=1e-4. This is
also the order used by the legacy laserbeamFoam Plate2D tutorial, while the raw
residual remains available in the performance diagnostics.

The next short A/B regression must verify:
1. a large reduction in thermal cap hits and corrector count;
2. stable raw bulk residual;
3. unchanged fusion-zone L/W/D to within mesh resolution;
4. no degradation of energy-source conservation.
