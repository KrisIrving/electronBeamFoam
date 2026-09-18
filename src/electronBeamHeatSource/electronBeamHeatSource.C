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
#include "constants.H"
#include <cmath>

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
    beamletRadialBins_(lookupOrDefault<label>("beamletRadialBins", 5)),
    beamletAngularBins_(lookupOrDefault<label>("beamletAngularBins", 12)),
    beamletRadiusFactor_
    (
        lookupOrDefault<scalar>("beamletRadiusFactor", 2.0)
    ),
    beamletMaxTrackHops_
    (
        lookupOrDefault<label>("beamletMaxTrackHops", 100000)
    ),
    multiRayMissAction_
    (
        lookupOrDefault<word>("multiRayMissAction", "skip")
    ),
    multiRayCacheEnabled_
    (
        lookupOrDefault<Switch>("multiRayCacheEnabled", false)
    ),
    multiRayRetraceInterval_
    (
        lookupOrDefault<label>("multiRayRetraceInterval", 10)
    ),
    multiRayRetraceDistanceFactor_
    (
        lookupOrDefault<scalar>("multiRayRetraceDistanceFactor", 0.05)
    ),
    cachedHitPositions_(),
    cachedHitDistances_(),
    cachedHitFound_(),
    cachedBeamletPowerFractions_(),
    cachedBeamPosition_(vector::zero),
    multiRayStepsSinceTrace_(0),
    multiRayCacheValid_(false),
    lastFirstHitPosition_(vector::zero),
    lastFirstHitDistance_(GREAT),
    lastFirstHitFound_(false),
    lastBeamletCount_(0),
    lastBeamletHitCount_(0),
    lastBeamletHitPowerFraction_(0.0),
    lastFirstHitMinDistance_(GREAT),
    lastFirstHitMaxDistance_(-GREAT),
    lastMultiRayRetraced_(false),
    lastMultiRayCacheAge_(0),
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
     && surfaceTrackingMode_ != "multiRayFirstHit"
    )
    {
        FatalErrorInFunction
            << "Unknown surfaceTrackingMode '" << surfaceTrackingMode_ << "'. "
            << "Valid modes are fixedReference, centralFirstHit and "
            << "multiRayFirstHit."
            << exit(FatalError);
    }

    if (firstHitSearchRadius_ <= SMALL)
    {
        FatalErrorInFunction
            << "firstHitSearchRadius must be positive" << exit(FatalError);
    }

    if (beamletRadialBins_ < 1 || beamletAngularBins_ < 1)
    {
        FatalErrorInFunction
            << "beamletRadialBins and beamletAngularBins must both be >= 1"
            << exit(FatalError);
    }

    if (beamletRadiusFactor_ <= SMALL)
    {
        FatalErrorInFunction
            << "beamletRadiusFactor must be positive" << exit(FatalError);
    }

    if (beamletMaxTrackHops_ < 1)
    {
        FatalErrorInFunction
            << "beamletMaxTrackHops must be >= 1" << exit(FatalError);
    }

    if (multiRayRetraceInterval_ < 1)
    {
        FatalErrorInFunction
            << "multiRayRetraceInterval must be >= 1" << exit(FatalError);
    }

    if (multiRayRetraceDistanceFactor_ < 0.0)
    {
        FatalErrorInFunction
            << "multiRayRetraceDistanceFactor must be >= 0" << exit(FatalError);
    }

    if
    (
        multiRayMissAction_ != "skip"
     && multiRayMissAction_ != "centralFallback"
    )
    {
        FatalErrorInFunction
            << "multiRayMissAction must be 'skip' or 'centralFallback'"
            << exit(FatalError);
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
        << "    beamletRadialBins    = " << beamletRadialBins_ << nl
        << "    beamletAngularBins   = " << beamletAngularBins_ << nl
        << "    beamletRadiusFactor  = " << beamletRadiusFactor_ << nl
        << "    multiRayMissAction   = " << multiRayMissAction_ << nl
        << "    multiRayCacheEnabled = " << multiRayCacheEnabled_ << nl
        << "    retraceInterval      = " << multiRayRetraceInterval_ << nl
        << "    retraceDistance      = "
        << multiRayRetraceDistanceFactor_ << " * beamRadius" << nl
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



void electronBeamHeatSource::transverseBasis(vector& u, vector& v) const
{
    const vector a =
        (mag(beamDirection_.z()) < 0.9)
      ? vector(0, 0, 1)
      : vector(0, 1, 0);

    u = beamDirection_ ^ a;
    u /= (mag(u) + VSMALL);

    v = beamDirection_ ^ u;
    v /= (mag(v) + VSMALL);
}


void electronBeamHeatSource::traceMultiRayFirstHits
(
    const volScalarField& alphaMetal,
    const vector& beamPosition,
    pointField& hitPositions,
    scalarField& hitDistances,
    labelList& hitFound,
    scalarField& beamletPowerFractions
) const
{
    const fvMesh& mesh = deposition_.mesh();
    const scalar pi = constant::mathematical::pi;

    const label nTotal = beamletRadialBins_*beamletAngularBins_;
    const scalar rMax = beamletRadiusFactor_*beamRadius_;

    pointField rayCoords(nTotal, point::zero);
    beamletPowerFractions.setSize(nTotal);
    beamletPowerFractions = 0.0;

    vector u(vector::zero), v(vector::zero);
    transverseBasis(u, v);

    scalarField radialBoundaries(beamletRadialBins_ + 1, 0.0);

    for (label iR = 0; iR <= beamletRadialBins_; ++iR)
    {
        radialBoundaries[iR] =
            rMax*scalar(iR)/scalar(beamletRadialBins_);
    }

    for (label iTheta = 0; iTheta < beamletAngularBins_; ++iTheta)
    {
        // Sample at the centre of each angular sector.
        const scalar theta =
            2.0*pi*(scalar(iTheta) + 0.5)/scalar(beamletAngularBins_);

        for (label iR = 0; iR < beamletRadialBins_; ++iR)
        {
            const label rayI = iTheta*beamletRadialBins_ + iR;
            const scalar r1 = radialBoundaries[iR];
            const scalar r2 = radialBoundaries[iR + 1];

            // Area-centroid radius for the annular sector.
            scalar r = 0.0;
            if (r2 > r1 + VSMALL)
            {
                r =
                    (2.0/3.0)
                   *(pow3(r2) - pow3(r1))
                   /(sqr(r2) - sqr(r1) + VSMALL);
            }

            rayCoords[rayI] =
                beamPosition
              + r*cos(theta)*u
              + r*sin(theta)*v;

            // Exact Gaussian fraction of this annular sector for
            // q(r)=2/(pi*rb^2)*exp(-2*r^2/rb^2).
            const scalar annulusFraction =
                exp(-2.0*sqr(r1/beamRadius_))
              - exp(-2.0*sqr(r2/beamRadius_));

            beamletPowerFractions[rayI] =
                annulusFraction/scalar(beamletAngularBins_);
        }
    }

    Cloud<electronFirstHitParticle> rayCloud
    (
        mesh,
        "electronFirstHitRays",
        IDLList<electronFirstHitParticle>()
    );

    // Move seed points slightly into the domain to avoid exact face hits.
    const scalar eps = max(1e-6*beamRadius_, scalar(100)*VSMALL);
    const vector perturbation = eps*(beamDirection_ + u);

    label localSeeded = 0;

    forAll(rayCoords, rayI)
    {
        const point seedPoint = rayCoords[rayI] + perturbation;
        const label cellI = mesh.findCell(seedPoint);

        if (cellI >= 0)
        {
            electronFirstHitParticle* pPtr = new electronFirstHitParticle
            (
                mesh,
                seedPoint,
                cellI,
                beamDirection_,
                rayI
            );

            rayCloud.addParticle(pPtr);
            ++localSeeded;
        }
    }

    label totalSeeded = localSeeded;
    reduce(totalSeeded, sumOp<label>());

    if (totalSeeded != nTotal && mesh.time().writeTime())
    {
        WarningInFunction
            << "Expected " << nTotal << " beamlets but seeded "
            << totalSeeded << ". Check that the complete beam footprint "
            << "starts inside the computational domain." << endl;
    }

    DynamicList<label> localHitIDs;
    DynamicList<point> localHitPositions;
    DynamicList<scalar> localHitDistances;
    label hopLimitTerminations = 0;

    electronFirstHitParticle::trackingData td
    (
        rayCloud,
        alphaMetal,
        firstHitAlphaCutoff_,
        beamletMaxTrackHops_,
        localHitIDs,
        localHitPositions,
        localHitDistances,
        hopLimitTerminations
    );

    rayCloud.storeGlobalPositions();
    rayCloud.move(rayCloud, td, GREAT);

    reduce(hopLimitTerminations, sumOp<label>());

    if (hopLimitTerminations > 0 && mesh.time().writeTime())
    {
        WarningInFunction
            << hopLimitTerminations
            << " beamlets reached beamletMaxTrackHops="
            << beamletMaxTrackHops_ << endl;
    }

    List<labelList> allIDs(Pstream::nProcs());
    List<pointField> allPositions(Pstream::nProcs());
    List<scalarField> allDistances(Pstream::nProcs());

    allIDs[Pstream::myProcNo()] = localHitIDs;
    allPositions[Pstream::myProcNo()] = localHitPositions;
    allDistances[Pstream::myProcNo()] = localHitDistances;

    Pstream::gatherList(allIDs);
    Pstream::gatherList(allPositions);
    Pstream::gatherList(allDistances);

    Pstream::broadcastList(allIDs);
    Pstream::broadcastList(allPositions);
    Pstream::broadcastList(allDistances);

    hitPositions = rayCoords;
    hitDistances.setSize(nTotal);
    hitDistances = GREAT;
    hitFound.setSize(nTotal);
    hitFound = 0;

    for (label procI = 0; procI < Pstream::nProcs(); ++procI)
    {
        const labelList& ids = allIDs[procI];
        const pointField& positions = allPositions[procI];
        const scalarField& distances = allDistances[procI];

        forAll(ids, i)
        {
            const label rayI = ids[i];

            if (rayI < 0 || rayI >= nTotal)
            {
                continue;
            }

            if (!hitFound[rayI] || distances[i] < hitDistances[rayI])
            {
                hitFound[rayI] = 1;
                hitPositions[rayI] = positions[i];
                hitDistances[rayI] = distances[i];
            }
        }
    }
}



void electronBeamHeatSource::updateDeposition
(
    const volScalarField& alphaMetal,
    const volVectorField& nFiltered
)
{
    const fvMesh& mesh = deposition_.mesh();
    const scalar time = mesh.time().value();
    const scalar pi = constant::mathematical::pi;

    const vector beamPosition = timeVsBeamPosition_(time);
    const scalar incidentPower = max(timeVsBeamPower_(time), scalar(0));
    const scalar requestedAbsorbedPower = absorptivity_*incidentPower;

    lastIncidentPower_ = incidentPower;
    lastAbsorbedPower_ = 0.0;
    lastFirstHitFound_ = false;
    lastFirstHitPosition_ = beamPosition;
    lastFirstHitDistance_ = GREAT;
    lastBeamletCount_ = 0;
    lastBeamletHitCount_ = 0;
    lastBeamletHitPowerFraction_ = 0.0;
    lastFirstHitMinDistance_ = GREAT;
    lastFirstHitMaxDistance_ = -GREAT;
    lastMultiRayRetraced_ = false;
    lastMultiRayCacheAge_ = multiRayStepsSinceTrace_;

    deposition_ = dimensionedScalar
    (
        "zero",
        deposition_.dimensions(),
        0.0
    );

    if (requestedAbsorbedPower <= SMALL)
    {
        deposition_.correctBoundaryConditions();

        if (mesh.time().writeTime())
        {
            Info<< "Electron beam: t=" << time << " s, beam off" << endl;
        }
        return;
    }

    vector depositionOrigin = beamPosition;

    pointField multiHitPositions;
    scalarField multiHitDistances;
    labelList multiHitFound;
    scalarField beamletPowerFractions;

    scalar effectiveAbsorbedPower = requestedAbsorbedPower;

    bool centralFallbackFound = false;
    vector centralFallbackPosition = beamPosition;
    scalar centralFallbackDistance = GREAT;

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
                    << "Electron deposition is zero for this update."
                    << endl;
            }
            return;
        }

        depositionOrigin = hitPosition;
        lastFirstHitFound_ = true;
        lastFirstHitPosition_ = hitPosition;
        lastFirstHitDistance_ = hitDistance;
        lastFirstHitMinDistance_ = hitDistance;
        lastFirstHitMaxDistance_ = hitDistance;
    }
    else if
    (
        depositionModel_ == "volumetricGaussian"
     && surfaceTrackingMode_ == "multiRayFirstHit"
    )
    {
        bool retrace = !multiRayCacheEnabled_ || !multiRayCacheValid_;

        if
        (
            multiRayCacheEnabled_
         && multiRayCacheValid_
         && multiRayStepsSinceTrace_ >= multiRayRetraceInterval_
        )
        {
            retrace = true;
        }

        if
        (
            multiRayCacheEnabled_
         && multiRayCacheValid_
         && multiRayRetraceDistanceFactor_ > 0.0
         && mag(beamPosition - cachedBeamPosition_)
            >= multiRayRetraceDistanceFactor_*beamRadius_
        )
        {
            retrace = true;
        }

        if (retrace)
        {
            traceMultiRayFirstHits
            (
                alphaMetal,
                beamPosition,
                multiHitPositions,
                multiHitDistances,
                multiHitFound,
                beamletPowerFractions
            );

            cachedHitPositions_ = multiHitPositions;
            cachedHitDistances_ = multiHitDistances;
            cachedHitFound_ = multiHitFound;
            cachedBeamletPowerFractions_ = beamletPowerFractions;
            cachedBeamPosition_ = beamPosition;
            multiRayStepsSinceTrace_ = 0;
            multiRayCacheValid_ = true;
            lastMultiRayRetraced_ = true;
            lastMultiRayCacheAge_ = 0;
        }
        else
        {
            multiHitPositions = cachedHitPositions_;
            multiHitDistances = cachedHitDistances_;
            multiHitFound = cachedHitFound_;
            beamletPowerFractions = cachedBeamletPowerFractions_;
            lastMultiRayRetraced_ = false;
            lastMultiRayCacheAge_ = multiRayStepsSinceTrace_;
        }

        // Counts CFD source updates since the most recent ray trace.
        ++multiRayStepsSinceTrace_;

        lastBeamletCount_ = multiHitFound.size();

        scalar totalSampledFraction = 0.0;
        scalar hitSampledFraction = 0.0;
        vector weightedHitPosition(vector::zero);
        scalar weightedHitDistance = 0.0;

        forAll(multiHitFound, rayI)
        {
            totalSampledFraction += beamletPowerFractions[rayI];

            if (multiHitFound[rayI])
            {
                ++lastBeamletHitCount_;
                hitSampledFraction += beamletPowerFractions[rayI];
                weightedHitPosition +=
                    beamletPowerFractions[rayI]*multiHitPositions[rayI];
                weightedHitDistance +=
                    beamletPowerFractions[rayI]*multiHitDistances[rayI];

                lastFirstHitMinDistance_ =
                    min(lastFirstHitMinDistance_, multiHitDistances[rayI]);
                lastFirstHitMaxDistance_ =
                    max(lastFirstHitMaxDistance_, multiHitDistances[rayI]);
            }
        }

        if (totalSampledFraction > VSMALL)
        {
            lastBeamletHitPowerFraction_ =
                hitSampledFraction/totalSampledFraction;
        }

        if (hitSampledFraction > VSMALL)
        {
            lastFirstHitFound_ = true;
            lastFirstHitPosition_ =
                weightedHitPosition/hitSampledFraction;
            lastFirstHitDistance_ =
                weightedHitDistance/hitSampledFraction;
        }

        if (!lastFirstHitFound_)
        {
            deposition_.correctBoundaryConditions();

            if (mesh.time().writeTime())
            {
                WarningInFunction
                    << "No multi-ray beamlet found metal at t=" << time
                    << " s. Electron deposition is zero." << endl;
            }
            return;
        }

        if (multiRayMissAction_ == "skip")
        {
            // Do not redistribute power represented by escaped beamlets.
            effectiveAbsorbedPower =
                requestedAbsorbedPower*lastBeamletHitPowerFraction_;
        }
        else if
        (
            lastBeamletHitCount_ < lastBeamletCount_
         && multiRayMissAction_ == "centralFallback"
        )
        {
            centralFallbackFound = locateCentralFirstHit
            (
                alphaMetal,
                nFiltered,
                beamPosition,
                centralFallbackPosition,
                centralFallbackDistance
            );

            if (!centralFallbackFound)
            {
                effectiveAbsorbedPower =
                    requestedAbsorbedPower*lastBeamletHitPowerFraction_;
            }
        }
    }

    const vectorField& Cc = mesh.C();
    const scalarField& V = mesh.V();
    const scalarField& alphaI = alphaMetal.primitiveField();

    scalarField rawWeight(mesh.nCells(), 0.0);

    tmp<volVectorField> tGradAlpha;
    if (depositionModel_ == "surfaceGaussian")
    {
        tGradAlpha = fvc::grad(alphaMetal);
    }

    const scalar invRb2 = 1.0/sqr(beamRadius_);

    const bool multiRayMode =
        depositionModel_ == "volumetricGaussian"
     && surfaceTrackingMode_ == "multiRayFirstHit";

    const scalar radialCutoff =
        multiRayMode
      ? beamletRadiusFactor_*beamRadius_
      : 4.0*beamRadius_;

    const scalar radialCutoff2 = sqr(radialCutoff);

    vector u(vector::zero), v(vector::zero);
    if (multiRayMode)
    {
        transverseBasis(u, v);
    }

    const scalar rMax = beamletRadiusFactor_*beamRadius_;

    forAll(rawWeight, celli)
    {
        scalar depth = 0.0;
        vector radial(vector::zero);
        scalar r2 = 0.0;

        if (multiRayMode)
        {
            const vector relBeam = Cc[celli] - beamPosition;
            const scalar axial = relBeam & beamDirection_;
            radial = relBeam - axial*beamDirection_;
            r2 = magSqr(radial);

            if (r2 > radialCutoff2 || alphaI[celli] < minMetalFraction_)
            {
                continue;
            }

            const scalar r = sqrt(max(r2, scalar(0)));
            scalar theta = std::atan2(radial & v, radial & u);

            if (theta < 0.0)
            {
                theta += 2.0*pi;
            }

            label iR = label
            (
                std::floor
                (
                    (r/(rMax + VSMALL))*scalar(beamletRadialBins_)
                )
            );
            iR = max(label(0), min(iR, beamletRadialBins_ - 1));

            label iTheta = label
            (
                std::floor
                (
                    (theta/(2.0*pi))*scalar(beamletAngularBins_)
                )
            );
            iTheta = max(label(0), min(iTheta, beamletAngularBins_ - 1));

            const label rayI = iTheta*beamletRadialBins_ + iR;

            if (multiHitFound[rayI])
            {
                depth =
                    (Cc[celli] - multiHitPositions[rayI]) & beamDirection_;
            }
            else if
            (
                multiRayMissAction_ == "centralFallback"
             && centralFallbackFound
            )
            {
                const point fallbackHit =
                    multiHitPositions[rayI]
                  + centralFallbackDistance*beamDirection_;

                depth = (Cc[celli] - fallbackHit) & beamDirection_;
            }
            else
            {
                continue;
            }
        }
        else
        {
            const vector rel = Cc[celli] - depositionOrigin;
            depth = rel & beamDirection_;
            radial = rel - depth*beamDirection_;
            r2 = magSqr(radial);

            if (r2 > radialCutoff2)
            {
                continue;
            }
        }

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
        if (mesh.time().writeTime())
        {
            WarningInFunction
                << "Electron beam does not intersect eligible metal/interface "
                << "cells at t=" << time << " s. Position=" << beamPosition
                << ", requested absorbed power=" << requestedAbsorbedPower
                << " W." << endl;
        }

        deposition_.correctBoundaryConditions();
        return;
    }

    scalarField& qI = deposition_.primitiveFieldRef();
    const scalar normalisation = effectiveAbsorbedPower/weightIntegral;

    forAll(qI, celli)
    {
        qI[celli] = normalisation*rawWeight[celli];
    }

    deposition_.correctBoundaryConditions();

    // By construction this is the globally normalised effective absorbed power.
    // Only integrate explicitly at write times to avoid a second MPI reduction
    // every CFD time step.
    lastAbsorbedPower_ = effectiveAbsorbedPower;

    if (mesh.time().writeTime())
    {
        lastAbsorbedPower_ = fvc::domainIntegrate(deposition_).value();

        Info<< "Electron beam: t=" << time << " s" << nl
            << "    position             = " << beamPosition << nl
            << "    surface tracking     = " << surfaceTrackingMode_ << nl;

        if (multiRayMode)
        {
            Info<< "    beamlets             = " << lastBeamletCount_ << nl
                << "    beamlets hit metal   = " << lastBeamletHitCount_ << nl
                << "    hit power fraction   = "
                << lastBeamletHitPowerFraction_ << nl
                << "    hit-map retraced     = "
                << lastMultiRayRetraced_ << nl
                << "    hit-map cache age    = "
                << lastMultiRayCacheAge_ << " updates" << nl
                << "    first-hit mean dist  = "
                << lastFirstHitDistance_ << " m" << nl
                << "    first-hit min dist   = "
                << lastFirstHitMinDistance_ << " m" << nl
                << "    first-hit max dist   = "
                << lastFirstHitMaxDistance_ << " m" << nl;
        }
        else
        {
            Info<< "    deposition origin    = " << depositionOrigin << nl
                << "    first-hit found      = " << lastFirstHitFound_ << nl
                << "    first-hit distance   = "
                << lastFirstHitDistance_ << " m" << nl;
        }

        Info<< "    incident power       = " << incidentPower << " W" << nl
            << "    requested absorbed   = " << requestedAbsorbedPower << " W" << nl
            << "    effective absorbed   = " << effectiveAbsorbedPower << " W" << nl
            << "    integrated deposited = " << lastAbsorbedPower_ << " W" << nl
            << "    power error          = "
            << lastAbsorbedPower_ - effectiveAbsorbedPower << " W" << endl;
    }
}

} // End namespace Foam
