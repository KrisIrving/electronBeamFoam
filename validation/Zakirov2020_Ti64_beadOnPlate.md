# Ti-6Al-4V bead-on-plate calibration target: Zakirov et al. (2020)

Primary source:

Andrey Zakirov et al., "Predictive modeling of laser and electron beam powder
bed fusion additive manufacturing of metals at the mesoscale",
Additive Manufacturing 35 (2020) 101236.
DOI: 10.1016/j.addma.2020.101236.

The paper is open access and contains electron-beam bead-on-plate experiments
for Ti-6Al-4V that are especially useful for electronBeamFoam because the
experiments directly report melt-track depth and width.

## Experimental target selected

Table 2 reports the following conditions for the 15 mA bead-on-plate cases:

- acceleration voltage: 60 kV;
- beam current: 15 mA;
- incident beam power: 900 W;
- nominal electron-beam spot size: D4sigma = 500 um;
- preheat temperature: 296 K or 973 K;
- scan speed: 1000, 3000 or 6000 mm/s.

The measured width/depth values are stored in
`Zakirov2020_Ti64_beadOnPlate.csv`.

The authors also report that a flat Ti-6Al-4V plate absorbs approximately 85%
of an incident 60 kV electron beam. This matches the current initial
electronBeamFoam absorptivity of 0.85.

## Gaussian radius convention

electronBeamFoam uses

```text
I(r) proportional to exp(-2 r^2 / beamRadius^2)
```

so `beamRadius` is the 1/e^2 intensity radius w. For an ideal circular
Gaussian beam, the ISO second-moment diameter D4sigma is 2w. Thus the nominal
D4sigma = 500 um target corresponds initially to:

```text
beamRadius = 250 um
```

The paper notes uncertainty in the actual ARCAM A2 spot size at higher current;
their numerical sensitivity study found about 600 um gave better agreement for
some 15 mA cases. This makes beam radius a calibration/sensitivity parameter,
not an exact known constant.

## Electron penetration depth

A separate published EBSM model using the same Ti-6Al-4V / 60 kV setting
reports an electron penetration scale of about 18.9 um. For the current
exponential volumetric source, 18.9-20 um is therefore a reasonable initial
`penetrationDepth`, but it is not equivalent to a full Monte-Carlo deposition
profile and remains a calibration parameter.

## Important numerical gate before calibration

The 2026-09-20 short moving-track regression showed that the legacy
laserbeamFoam phase-change corrector reached its maximum correction count on
nearly every time step, with an O(1) final global epsilon residual. Therefore
these experimental targets are now recorded, but production fitting of
absorptivity / beamRadius / penetrationDepth must wait until the residual
location is identified and the thermal/phase solve is numerically defensible.

The next short run should use the region-resolved convergence diagnostics added
in `docs/PHASE_CHANGE_CONVERGENCE.md`.
