IOdictionary solutionDict
(
    IOobject
    (
        "fvSolution",
        runTime.system(),
        runTime,
        IOobject::MUST_READ
    )
);

dictionary& meltingDict = solutionDict.subDict("MELTING");

const label minTCorr(readLabel(meltingDict.lookup("minTempCorrector")));

const label maxTCorr(readLabel(meltingDict.lookup("maxTempCorrector")));

const dimensionedScalar epsilonTol
(
    readScalar(meltingDict.lookup("epsilonTolerance"))
);

const dimensionedScalar epsilonRel
(
    readScalar(meltingDict.lookup("epsilonRelaxation"))
);

// EPBF interface-consistent phase-change correction.
// Legacy mode retains the upstream laserbeamFoam mixture interpolation.
// Consistent mode treats epsilon1 as the metal liquid fraction in any
// metal-containing VOF cell while leaving pure numerical-void cells on the
// legacy gas branch (epsilon1 ~ 1, so they are not Darcy-damped).
const bool consistentMetalPhaseChange =
    meltingDict.lookupOrDefault<bool>
    (
        "consistentMetalPhaseChange",
        false
    );

const scalar phaseChangeMetalFractionCutoff =
    meltingDict.lookupOrDefault<scalar>
    (
        "phaseChangeMetalFractionCutoff",
        0.01
    );

const bool damperSwitch =
    meltingDict.lookupOrDefault<bool>("damperSwitch", false);
