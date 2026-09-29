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

## Gate V0 result — ACCEPTED FOR CURRENT BASELINE

Completed 0.5 mm / 296 K / 900 W / 3 m/s simpleXZ case:

- equivalent evaporated metal volume: about 5.57e-15 m3;
- fusion-zone volume: about 5.812e-12 m3;
- evaporation/fusion volume: about 0.096%;
- final evaporation power: about 2.95 W, or 0.386% of the 765 W absorbed beam power;
- a deliberately conservative upper bound obtained by holding the final peak
  recession speed for the entire scan is about 0.57 um;
- the graded-y interface base cell is 6.25 um and maxRefinement=2 gives a
  nominal finest y scale of about 1.56 um.

Decision: do not add explicit VOF mass removal before the remaining vacuum
fidelity gates. This decision applies to the present calibration regime and
must be revisited for hotter/deeper regimes where evaporation grows strongly.

Formal archive:
`validation/Zakirov2020_vacuumFidelity_V0.json`.

## Gate V1 — numerical-void thermal transport

The current numerical void still has finite thermal conductivity and heat
capacity. This can create an artificial conductive heat-loss path in a vacuum.

Hold all metal, beam and interface physics fixed and vary numerical-void
thermal conductivity first:

- current k_void = 0.04 W/(m K), scale 1;
- scale 0.1 -> 0.004 W/(m K);
- scale 0.01 -> 0.0004 W/(m K).

The completed V0 case is archived locally as the exact 1x baseline before the
candidate runs start. Run the controlled sweep with:

    ./Run_preflight kappa

The harness then runs only the 0.1x and 0.01x candidates, archives exact CSV
and JSON summaries for all three cases under
`vacuumFidelity/V1_kappa/`, and writes `comparison.txt`.

Compare W/D/L/V, Tmax, recoil, evaporation mass, energy loss and numerical
stability. Do not change the physical radiation or evaporation terms.

## Gate V1 result — REVIEW, extend to V1b

The 1x / 0.1x / 0.01x sweep showed:

- W/D/L unchanged over the full 100x reduction;
- fusion-zone volume changed only +0.17% to +0.26%;
- at 0.01x, fusion peak temperature increased 1.153%;
- melt-pool Tmax increased 0.854%;
- recoil pressure increased about 15.99%;
- evaporation remained absolutely small (about 0.096% of fusion volume and
  about 0.4% of absorbed beam power).

Therefore the current 0.04 W/(m K) void conductivity is not yet demonstrated
to be a harmless thermal regularisation. Geometry is insensitive, but
temperature-sensitive surface physics is not demonstrably converged.

Formal archive:
`validation/Zakirov2020_vacuumFidelity_V1.json`.

### Gate V1b — low-k convergence toward the vacuum limit

Before V2, extend the conductivity sequence:

- 0.01x  = 4e-4 W/(m K) (already available);
- 0.001x = 4e-5 W/(m K);
- 0.0001x = 4e-6 W/(m K).

Run:

    ./Run_preflight kappa-limit

The last conductivity decade (0.001x -> 0.0001x) is the predeclared plateau
gate:

- W/D/L <= 0.25%;
- fusion volume <= 0.5%;
- fusion and melt Tmax <= 0.5%;
- recoil <= 5%;
- evaporation power <= 2%;
- absolute evaporation remains <=0.5% of fusion volume and <=1% of absorbed
  beam power.

If this final decade passes, use the *largest* conductivity already inside the
plateau, 0.001x, as the production numerical regularisation. This avoids an
unnecessarily extreme coefficient while approximating the zero-conduction
vacuum limit.

If it does not pass, do not proceed to V2; revisit the thermal treatment of
the numerical void explicitly.

## Gate V1b result — REVIEW, no low-k plateau

The completed 0.01x -> 0.001x -> 0.0001x sequence showed that geometry was
already insensitive, but temperature-sensitive surface physics was not:

- final-decade W/D/L change: 0%;
- fusion volume: +0.081%;
- fusion peak temperature: +0.478%;
- melt Tmax: +0.628%;
- recoil pressure: +11.535%;
- evaporation power: +0.573%.

The predeclared Tmax/recoil plateau gate therefore failed. Continuing to reduce
the cell-centred void conductivity is not justified.

Formal archive:
`validation/Zakirov2020_vacuumFidelity_V1b.json`.

### Why V1b did not solve the thermal-void problem

The thermal equation currently contains

    -fvm::laplacian(kappa, T)

with cell-centred

    kappa = alpha*k_metal + (1-alpha)*k_void.

The default laplacian scheme is `Gauss linear corrected`. OpenFOAM therefore
interpolates the cell-centred diffusion coefficient to faces using a linear
scheme. Even as the pure-void cell value approaches zero, a face connecting an
interface cell to a void cell can retain a finite diffusion coefficient because
the interface-cell conductivity still contains a metal fraction.

For a numerical vacuum this is undesirable: the secondary phase must remain
usable by VOF, but it should not act as an ordinary continuum conductive heat
sink from the metal free surface.

### Gate V1c — harmonic face conductivity

OpenFOAM provides a harmonic-mean surface interpolation scheme. Test it without
changing the cell-property model or any physical surface-loss term.

Run:

    ./Run_preflight thermal-face

The controlled 0.5 mm sweep uses harmonic coefficient interpolation for
`laplacian(kappa,T)` and tests:

- 1x:    k_void = 0.04 W/(m K);
- 0.1x:  k_void = 0.004 W/(m K);
- 0.01x: k_void = 0.0004 W/(m K).

The final decade (0.1x -> 0.01x) uses the same plateau criteria as V1b:

- W/D/L <=0.25%;
- V <=0.5%;
- Tmax <=0.5%;
- recoil <=5%;
- Qevap <=2%;
- absolute evaporation remains <=0.5% fusion volume and <=1% absorbed power.

The comparison also reports the change from the legacy linear-face 1x case to
the harmonic-face 1x case. If the harmonic low-k sequence passes, select the
largest k_void inside the plateau for numerical robustness and then proceed to
V2. If it fails, isolate the energy formulation further before hydrodynamic
void-property testing.

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
