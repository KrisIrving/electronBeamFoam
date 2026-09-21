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
