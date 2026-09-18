MRF.correctBoundaryVelocity(U);


if (electronBeam.powderSim())
{
    DC =
        DarcyConstantlarge*Foam::pow((1.0-epsilon1mask),2)
       /(Foam::pow(epsilon1mask, 3) + DarcyConstantsmall);
}
else
{
    DC =
        DarcyConstantlarge*Foam::pow((1.0 - epsilon1), 2)
       /(Foam::pow(epsilon1, 3) + DarcyConstantsmall);
}

DC.correctBoundaryConditions();  // ADDED

gradAlpha = fvc::grad(alpha1);
gradAlpha.correctBoundaryConditions();  // ADDED - CRITICAL!

nHatM = gradAlpha/(mag(gradAlpha) + deltaN);
nHatM.correctBoundaryConditions();  // ADDED

Marangoni =
    epsilon1*Marangoni_Constant*(gradT - nHatM*(nHatM & gradT))*mag(gradAlpha);

const scalar Tliquidus1Value = Tliquidus1.value();
const scalarField& TI = T;
vectorField& MarangoniI = Marangoni;
forAll(MarangoniI, celli)
{
    if(TI[celli] < Tliquidus1Value)
    {
        MarangoniI[celli] = vector::zero;
    }
}

Marangoni.correctBoundaryConditions();

if (damperSwitch)
{
    // To account for high density difference
    damper = 2.0*rho/(rho1 + rho2);
    damper.correctBoundaryConditions();  // ADDED
}

pSat = p0*Foam::exp(LatentHeatVap*Mm*((T - Tvap)/(R*T*Tvap)));
pSat.correctBoundaryConditions();

// Net recoil pressure above the chamber background pressure.
pVap = recoilCoefficient*max
(
    pSat - pAmbient,
    dimensionedScalar("zeroPressure", p0.dimensions(), 0.0)
);
pVap.correctBoundaryConditions();


if (mthd.valid())
{
    mthd->updateLorentzForce();
}

const volVectorField& JcrossB =
    mthd.valid()
  ? mthd->JcrossB()
  : zeroJcrossB;

fvVectorMatrix UEqn
(
    fvm::ddt(rho, U) + fvm::div(rhoPhi, U)
  + MRF.DDt(rho, U)
  + turbulence->divDevRhoReff(rho, U)
  + fvm::Sp(DC, U)
  - Marangoni*damper
  - JcrossB*damper
 ==
    fvOptions(rho, U)
);

UEqn.relax();

fvOptions.constrain(UEqn);

if (pimple.momentumPredictor())
{
    solve
    (
        UEqn
     ==
        fvc::reconstruct
        (
            (
                mixture.surfaceTensionForce()*fvc::interpolate(damper)
                + (fvc::interpolate(pVap)
                    *fvc::snGrad(alpha1)
                    *fvc::interpolate(damper))
              - ghf*fvc::snGrad(rho*rhok)
              - fvc::snGrad(p_rgh)
            ) * mesh.magSf()
        )
    );

    fvOptions.correct(U);
}
