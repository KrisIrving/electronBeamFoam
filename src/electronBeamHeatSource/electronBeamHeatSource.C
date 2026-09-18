/*---------------------------------------------------------------------------*\
    Electron-beam heat source for electronBeamFoam.

    Immediately usable deposition models:
      surfaceGaussian
      volumetricGaussian

    The deposited field is mesh-normalised each update so that the integrated
    source equals absorptivity*incidentPower whenever the beam intersects
    eligible cells.
\*---------------------------------------------------------------------------*/

#include "electronBeamHeatSource.H"
#include "fvCFD.H"

namespace Foam
{

defineTypeNameAndDebug(electronBeamHeatSource, 0);

electronBeamHeatSource::electronBeamHeatSource(const fvMesh& mesh)
:
    IOdictionary
    (
        IOobject
        (
            "ElectronBeamProperties",
            mesh.time().constant(),
            mesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        )
    ),
    deposition_
    (
        IOobject
        (
            "ElectronDeposition",
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar
        (
            "electronDeposition",
            dimensionSet(1, -1, -3, 0, 0),
            0.0
        )
    ),
    powderSim_(lookupOrDefault<Switch>("PowderSim", false)),
    depositionModel_
    (
        lookupOrDefault<word>("depositionModel", "volumetricGaussian")
    ),
    absorptivity_(lookupOrDefault<scalar>("absorptivity", 0.85)),
    beamRadius_(readScalar(lookup("beamRadius"))),
    penetrationDepth_(lookupOrDefault<scalar>("penetrationDepth", 20e-6)),
    maxPenetrationDepth_
    (
        lookupOrDefault<scalar>
        (
            "maxPenetrationDepth",
            6.0*penetrationDepth_
        )
    ),
    minMetalFraction_(lookupOrDefault<scalar>("minMetalFraction", 1e-3)),
    beamDirection_(lookup("beamDirection")),
    surfaceTrackingMode_
    (
        lookupOrDefault<word>("surfaceTrackingMode", "centralFirstHit")
    ),
    firstHitSearchRadius_
    (
        lookupOrDefault<scalar>("firstHitSearchRadius", 0.5*beamRadius_)
    ),
    firstHitNormalThreshold_
    (
        lookupOrDefault<scalar>("firstHitNormalThreshold", 0.1)
    ),
    firstHitAlphaCutoff_
    (
        lookupOrDefault<scalar>("firstHitAlphaCutoff", 1e-3)
    ),
    firstHitFallbackMetalFraction_
    (
        lookupOrDefault<scalar>("firstHitFallbackMetalFraction", 0.5)
    ),
    firstHitFallbackToMetal_
    (
        lookupOrDefault<Switch>("firstHitFallbackToMetal", true)
    ),
    lastFirstHitPosition_(vector::zero),
    lastFirstHitDistance_(GREAT),
    lastFirstHitFound_(false),
    timeVsBeamPosition_(subDict("timeVsBeamPosition")),
    timeVsBeamPower_(subDict("timeVsBeamPower")),
    lastIncidentPower_(0.0),
    lastAbsorbedPower_(0.0)
{
    const scalar dirMag = mag(beamDirection_);

    if (dirMag < SMALL)
    {
        FatalErrorInFunction
            << "beamDirection must be non-zero" << exit(FatalError);
    }

    beamDirection_ /= dirMag;

    if (beamRadius_ <= SMALL)
    {
        FatalErrorInFunction
            << "beamRadius must be positive" << exit(FatalError);
    }

    if
    (
        depositionModel_ != "surfaceGaussian"
     && depositionModel_ != "volumetricGaussian"
     && depositionModel_ != "tabulatedMC"
    )
    {
        FatalErrorInFunction
            << "Unknown depositionModel '" << depositionModel_ << "'. "
            << "Valid models are surfaceGaussian, volumetricGaussian, "
            << "and tabulatedMC."
            << exit(FatalError);
    }

    if
    (
        surfaceTrackingMode_ != "fixedReference"
     && surfaceTrackingMode_ != "centralFirstHit"
    )
    {
        FatalErrorInFunction
            << "Unknown surfaceTrackingMode '" << surfaceTrackingMode_ << "'. "
            << "Valid modes are fixedReference and centralFirstHit."
            << exit(FatalError);
    }

    if (firstHitSearchRadius_ <= SMALL)
    {
        FatalErrorInFunction
            << "firstHitSearchRadius must be positive" << exit(FatalError);
    }

    if
    (
        depositionModel_ == "volumetricGaussian"
     && penetrationDepth_ <= SMALL
    )
    {
        FatalErrorInFunction
            << "penetrationDepth must be positive for volumetricGaussian"
            << exit(FatalError);
    }

    if (depositionModel_ == "tabulatedMC")
    {
        FatalErrorInFunction
            << "tabulatedMC is reserved in the architecture but the MC table "
            << "reader is not implemented in this first code drop. "
            << "Use volumetricGaussian for bare-plate calibration."
            << exit(FatalError);
    }

    Info<< "Electron-beam heat source" << nl
        << "    depositionModel      = " << depositionModel_ << nl
        << "    absorptivity         = " << absorptivity_ << nl
        << "    beamRadius           = " << beamRadius_ << " m" << nl
        << "    penetrationDepth     = " << penetrationDepth_ << " m" << nl
        << "    maxPenetrationDepth  = " << maxPenetrationDepth_ << " m" << nl
        << "    beamDirection        = " << beamDirection_ << nl
        << "    surfaceTrackingMode  = " << surfaceTrackingMode_ << nl
        << "    firstHitSearchRadius = " << firstHitSearchRadius_ << " m" << nl
        << "    PowderSim            = " << powderSim_ << endl;
}


bool electronBeamHeatSource::locateCentralFirstHit
(
    const volScalarField& alphaMetal,
    const volVectorField& nFiltered,
    const vector& beamPosition,
    vector& hitPosition,
    scalar& hitDistance
) const
{
    const fvMesh& mesh = deposition_.mesh();
    const vectorField& C = mesh.C();
    const scalarField& V = mesh.V();
    const scalarField& alphaI = alphaMetal.primitiveField();
    const vectorField& nI = nFiltered.primitiveField();

    const scalar searchRadius2 = sqr(firstHitSearchRadius_);
    scalar localHitDistance = GREAT;

    // First preference: cells carrying a resolved metal-vacuum interface.
    // The half-cell correction moves the cell-centre estimate towards the
    // upstream face and removes the systematic O(dx/2) penetration offset
    // that otherwise appears on initially sharp setFields interfaces.
    forAll(C, celli)
    {
        const vector rel = C[celli] - beamPosition;
        const scalar s = rel & beamDirection_;

        if (s < 0.0)
        {
            continue;
        }

        const vector radial = rel - s*beamDirection_;
        if (magSqr(radial) > searchRadius2)
        {
            continue;
        }

        if
        (
            alphaI[celli] >= firstHitAlphaCutoff_
         && mag(nI[celli]) >= firstHitNormalThreshold_
        )
        {
            const scalar halfCell = 0.5*cbrt(V[celli]);
            localHitDistance = min(localHitDistance, max(s - halfCell, scalar(0)));
        }
    }

    reduce(localHitDistance, minOp<scalar>());

    // On an exactly sharp initial VOF field the interface-normal criterion can
    // occasionally miss the first metal cell.  Fall back to the first bulk
    // metal cell on the beam axis, again with a half-cell correction.
    if
    (
        localHitDistance >= 0.5*GREAT
     && firstHitFallbackToMetal_
    )
    {
        scalar localMetalDistance = GREAT;

        forAll(C, celli)
        {
            if (alphaI[celli] < firstHitFallbackMetalFraction_)
            {
                continue;
            }

            const vector rel = C[celli] - beamPosition;
            const scalar s = rel & beamDirection_;

            if (s < 0.0)
            {
                continue;
            }

            const vector radial = rel - s*beamDirection_;
            if (magSqr(radial) > searchRadius2)
            {
                continue;
            }

            const scalar halfCell = 0.5*cbrt(V[celli]);
            localMetalDistance = min
            (
                localMetalDistance,
                max(s - halfCell, scalar(0))
            );
        }

        reduce(localMetalDistance, minOp<scalar>());
        localHitDistance = localMetalDistance;
    }

    if (localHitDistance >= 0.5*GREAT)
    {
        hitDistance = GREAT;
        hitPosition = beamPosition;
        return false;
    }

    hitDistance = localHitDistance;
    hitPosition = beamPosition + hitDistance*beamDirection_;
    return true;
}


void electronBeamHeatSource::updateDeposition
(
    const volScalarField& alphaMetal,
    const volVectorField& nFiltered
)
{
    const fvMesh& mesh = deposition_.mesh();
    const scalar time = mesh.time().value();

    const vector beamPosition = timeVsBeamPosition_(time);
    const scalar incidentPower = max(timeVsBeamPower_(time), scalar(0));
    const scalar targetAbsorbedPower = absorptivity_*incidentPower;

    lastIncidentPower_ = incidentPower;
    lastAbsorbedPower_ = 0.0;

    deposition_ = dimensionedScalar
    (
        "zero",
        deposition_.dimensions(),
        0.0
    );

    if (targetAbsorbedPower <= SMALL)
    {
        deposition_.correctBoundaryConditions();
        lastFirstHitFound_ = false;
        lastFirstHitPosition_ = beamPosition;
        lastFirstHitDistance_ = GREAT;

        // Avoid per-time-step console/file I/O.  Report only at write times.
        if (mesh.time().writeTime())
        {
            Info<< "Electron beam: t=" << time << " s, beam off" << endl;
        }
        return;
    }

    vector depositionOrigin = beamPosition;
    lastFirstHitFound_ = false;
    lastFirstHitPosition_ = beamPosition;
    lastFirstHitDistance_ = 0.0;

    if
    (
        depositionModel_ == "volumetricGaussian"
     && surfaceTrackingMode_ == "centralFirstHit"
    )
    {
        vector hitPosition(vector::zero);
        scalar hitDistance = GREAT;

        if
        (
            !locateCentralFirstHit
            (
                alphaMetal,
                nFiltered,
                beamPosition,
                hitPosition,
                hitDistance
            )
        )
        {
            deposition_.correctBoundaryConditions();

            if (mesh.time().writeTime())
            {
                WarningInFunction
                    << "No central first-hit metal surface was found at t="
                    << time << " s. beamPosition=" << beamPosition
                    << ", searchRadius=" << firstHitSearchRadius_ << " m. "
                    << "Electron deposition is set to zero for this update."
                    << endl;
            }
            return;
        }

        depositionOrigin = hitPosition;
        lastFirstHitFound_ = true;
        lastFirstHitPosition_ = hitPosition;
        lastFirstHitDistance_ = hitDistance;
    }

    const vectorField& C = mesh.C();
    const scalarField& V = mesh.V();
    const scalarField& alphaI = alphaMetal.primitiveField();

    scalarField rawWeight(mesh.nCells(), 0.0);

    tmp<volVectorField> tGradAlpha;
    if (depositionModel_ == "surfaceGaussian")
    {
        tGradAlpha = fvc::grad(alphaMetal);
    }

    const scalar invRb2 = 1.0/sqr(beamRadius_);

    // A Gaussian at r=4*rb has exp(-32) weight (~1.3e-14).  Skipping
    // cells outside this radius avoids expensive exp() evaluations without
    // changing the deposited energy, since the retained kernel is globally
    // normalised below.
    const scalar radialCutoff2 = 16.0*sqr(beamRadius_);

    forAll(rawWeight, celli)
    {
        const vector rel = C[celli] - depositionOrigin;
        const scalar depth = rel & beamDirection_;

        // For the volumetric model, reject cells before evaluating the
        // radial Gaussian.  This is important for large parallel meshes.
        if
        (
            depositionModel_ == "volumetricGaussian"
         &&
            (
                alphaI[celli] < minMetalFraction_
             || depth < 0.0
             || depth > maxPenetrationDepth_
            )
        )
        {
            continue;
        }

        const vector radial = rel - depth*beamDirection_;
        const scalar r2 = magSqr(radial);

        if (r2 > radialCutoff2)
        {
            continue;
        }

        const scalar radialWeight = exp(-2.0*r2*invRb2);

        if (depositionModel_ == "surfaceGaussian")
        {
            const scalar gradAlphaMag =
                mag(tGradAlpha().primitiveField()[celli]);

            rawWeight[celli] = radialWeight*gradAlphaMag;
        }
        else if (depositionModel_ == "volumetricGaussian")
        {
            rawWeight[celli] =
                alphaI[celli]
               *radialWeight
               *exp(-depth/penetrationDepth_);
        }
    }

    scalar weightIntegral = 0.0;
    forAll(rawWeight, celli)
    {
        weightIntegral += rawWeight[celli]*V[celli];
    }
    reduce(weightIntegral, sumOp<scalar>());

    if (weightIntegral <= VSMALL)
    {
        // Do not flood long parallel runs with the same warning every time
        // step.  A write-time warning is sufficient for diagnostics.
        if (mesh.time().writeTime())
        {
            WarningInFunction
                << "Electron beam does not intersect eligible metal/interface "
                << "cells at t=" << time << " s. Position=" << beamPosition
                << ", requested absorbed power=" << targetAbsorbedPower << " W."
                << endl;
        }

        deposition_.correctBoundaryConditions();
        return;
    }

    scalarField& qI = deposition_.primitiveFieldRef();
    const scalar normalisation = targetAbsorbedPower/weightIntegral;

    forAll(qI, celli)
    {
        qI[celli] = normalisation*rawWeight[celli];
    }

    deposition_.correctBoundaryConditions();

    // The field is normalised using the globally reduced weightIntegral, so
    // its integral is targetAbsorbedPower by construction.  Avoid a second
    // MPI global reduction every time step.  Perform the explicit integral
    // only at output times as a power-conservation diagnostic.
    lastAbsorbedPower_ = targetAbsorbedPower;

    if (mesh.time().writeTime())
    {
        lastAbsorbedPower_ = fvc::domainIntegrate(deposition_).value();

        Info<< "Electron beam: t=" << time << " s" << nl
            << "    position             = " << beamPosition << nl
            << "    deposition origin    = " << depositionOrigin << nl
            << "    first-hit found      = " << lastFirstHitFound_ << nl
            << "    first-hit distance   = " << lastFirstHitDistance_ << " m" << nl
            << "    incident power       = " << incidentPower << " W" << nl
            << "    target absorbed      = " << targetAbsorbedPower << " W" << nl
            << "    integrated deposited = " << lastAbsorbedPower_ << " W" << nl
            << "    power error          = "
            << lastAbsorbedPower_ - targetAbsorbedPower << " W" << endl;
    }
}

} // End namespace Foam
