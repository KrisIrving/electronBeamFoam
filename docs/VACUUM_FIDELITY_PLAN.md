# Vacuum Fidelity Verification

This plan determines whether the current EPBF chamber treatment is adequate
for melt-pool validation before any more source-parameter calibration.

## Model being tested

electronBeamFoam currently uses:

- a two-phase incompressible VOF formulation;
- the secondary phase as a numerical void, not a continuum rarefied gas;
- chamber pressure `pAmbient` only in the recoil-pressure closure;
- ambient metal-vapour partial pressure `pVaporAmbient` in the
  Hertz-Knudsen-type evaporation closure;
- radiation, evaporation, recoil, Marangoni and surface tension at the
  metal/void interface.

The purpose of this stage is not to add a rarefied-gas solver. It is to prove
that the numerical-void regularisation does not control the predicted melt pool
and to quantify model terms that may invalidate the present approximation.

## Gate V0 — implied evaporative mass loss

The energy equation already removes evaporation latent heat, but the VOF
equations do not remove metal mass. The diagnostic introduced in
`evaporationDiagnostics.H` converts the applied evaporation cooling into

    mDot'' = Qv / Lv

and integrates it with the same diffuse-interface weighting used by TEqn.

Outputs:

- instantaneous evaporation mass rate;
- cumulative implied evaporated mass;
- equivalent removed metal volume;
- evaporation power;
- maximum local mass flux;
- maximum local recession speed;
- geometric metal/void interface area.

The cumulative mass-density field is AUTO_WRITE and registered with the mesh
when diagnostics are enabled, so restart and AMR mapping preserve the history.

First run:

    ./Run_vacuumFidelityBaseline

Acceptance decision is based on scale, not an arbitrary percentage alone.
Compare the equivalent removed volume and implied recession length with the
finest interface-cell scale and with the fusion-zone volume. If the implied
surface recession is not small relative to the resolved interface scale,
explicit evaporative mass removal becomes a required physics development.

## Gate V1 — numerical-void thermal transport

The current numerical void still has finite thermal conductivity and heat
capacity. This can create an artificial conductive heat-loss path in a vacuum.

Hold all metal, beam and interface physics fixed and vary numerical-void
thermal conductivity first. At minimum test:

- current k_void;
- 0.1 x current k_void;
- 0.01 x current k_void.

Compare W/D/L/V, Tmax, recoil, evaporation mass, energy loss and numerical
stability. Do not change the physical radiation or evaporation terms.

## Gate V2 — numerical-void hydrodynamic regularisation

After V1, independently perturb:

- void density;
- void viscosity.

The objective is not to assign physical 0.2 Pa gas properties. It is to show
that the regularisation values needed by incompressible VOF do not materially
control the metal solution.

## Gate V3 — chamber and metal-vapour pressure closures

With void-property sensitivity bounded, test the closures themselves:

- chamber `pAmbient` sensitivity in recoil pressure;
- `pVaporAmbient` sensitivity in evaporation;
- analytical/standalone checks of saturation pressure, evaporation mass flux
  and recoil force.

A non-zero local metal-vapour back pressure may eventually require an
effective closure, but this is separate from the numerical void.

## Gate V4 — decision on explicit mass transfer

Only after V0-V3 decide between:

1. retain diagnostic-only evaporation mass loss when the implied interface
   recession is demonstrably unresolved/negligible for target conditions; or
2. implement conservative metal mass removal and VOF interface recession.

Do not implement explicit mass transfer merely because it is more elaborate.

## Exit condition

The numerical-vacuum model is accepted for EPBF melt-pool validation only when:

- melt-pool observables are insensitive to reasonable numerical-void
  regularisation changes;
- evaporation/recoil closures pass their verification checks;
- implied omitted mass loss is quantitatively bounded;
- the model remains stable under AMR, parallel restart and the accepted
  simpleXZ 48-rank workflow.

After this gate closes, return to source-shape/penetration-depth sensitivity
and the multi-condition Zakirov validation campaign.
