/*---------------------------------------------------------------------------*\
License
    This file is part of electronBeamFoam, derived from laserbeamFoam.
\*---------------------------------------------------------------------------*/

#ifndef electronBeamHeatSource_H
#define electronBeamHeatSource_H

#include "volFields.H"
#include "dictionary.H"
#include "interpolationTable.H"
#include "Cloud.H"
#include "electronFirstHitParticle.H"

namespace Foam
{

class electronBeamHeatSource
:
    public IOdictionary
{
    volScalarField deposition_;
    const Switch powderSim_;

    word depositionModel_;
    scalar absorptivity_;
    scalar beamRadius_;
    scalar penetrationDepth_;
    scalar maxPenetrationDepth_;
    scalar minMetalFraction_;
    vector beamDirection_;

    // Reference-surface handling for volumetric deposition.  In EPBF the
    // beam path is normally specified in the vacuum region above the workpiece,
    // so the electron penetration depth must be measured from the instantaneous
    // metal-vacuum surface, not from the tabulated beam-position point.
    word surfaceTrackingMode_;
    scalar firstHitSearchRadius_;
    scalar firstHitNormalThreshold_;
    scalar firstHitAlphaCutoff_;
    scalar firstHitFallbackMetalFraction_;
    Switch firstHitFallbackToMetal_;

    // Polar multi-ray surface map controls.
    label beamletRadialBins_;
    label beamletAngularBins_;
    scalar beamletRadiusFactor_;
    label beamletMaxTrackHops_;
    word multiRayMissAction_;

    // Optional cache for the expensive Lagrangian first-hit map.
    Switch multiRayCacheEnabled_;
    label multiRayRetraceInterval_;
    scalar multiRayRetraceDistanceFactor_;

    pointField cachedHitPositions_;
    scalarField cachedHitDistances_;
    labelList cachedHitFound_;
    scalarField cachedBeamletPowerFractions_;
    vector cachedBeamPosition_;
    label multiRayStepsSinceTrace_;
    Switch multiRayCacheValid_;

    vector lastFirstHitPosition_;
    scalar lastFirstHitDistance_;
    Switch lastFirstHitFound_;

    label lastBeamletCount_;
    label lastBeamletHitCount_;
    scalar lastBeamletHitPowerFraction_;
    scalar lastFirstHitMinDistance_;
    scalar lastFirstHitMaxDistance_;
    Switch lastMultiRayRetraced_;
    label lastMultiRayCacheAge_;

    interpolationTable<vector> timeVsBeamPosition_;
    interpolationTable<scalar> timeVsBeamPower_;

    scalar lastIncidentPower_;
    scalar lastAbsorbedPower_;

    electronBeamHeatSource(const electronBeamHeatSource&);
    void operator=(const electronBeamHeatSource&);

    //- Locate the first metal-vacuum intersection of the central beam axis.
    //  The search is performed independently on each MPI rank and the minimum
    //  positive beam-wise distance is reduced globally.  The returned position
    //  lies on the central beam axis.
    bool locateCentralFirstHit
    (
        const volScalarField& alphaMetal,
        const volVectorField& nFiltered,
        const vector& beamPosition,
        vector& hitPosition,
        scalar& hitDistance
    ) const;

    //- Construct an orthonormal transverse basis around beamDirection_.
    void transverseBasis(vector& u, vector& v) const;

    //- Trace a polar set of beamlets to the first metal-containing cell.
    //  Hit arrays are reconstructed identically on all MPI ranks.
    void traceMultiRayFirstHits
    (
        const volScalarField& alphaMetal,
        const vector& beamPosition,
        pointField& hitPositions,
        scalarField& hitDistances,
        labelList& hitFound,
        scalarField& beamletPowerFractions
    ) const;

public:
    TypeName("electronBeamHeatSource");

    explicit electronBeamHeatSource(const fvMesh& mesh);
    virtual ~electronBeamHeatSource() {}

    const volScalarField& deposition() const { return deposition_; }
    const Switch& powderSim() const { return powderSim_; }
    scalar lastIncidentPower() const { return lastIncidentPower_; }
    scalar lastAbsorbedPower() const { return lastAbsorbedPower_; }
    bool lastFirstHitFound() const { return lastFirstHitFound_; }
    const vector& lastFirstHitPosition() const { return lastFirstHitPosition_; }
    scalar lastFirstHitDistance() const { return lastFirstHitDistance_; }

    label lastBeamletCount() const { return lastBeamletCount_; }
    label lastBeamletHitCount() const { return lastBeamletHitCount_; }
    scalar lastBeamletHitPowerFraction() const
    {
        return lastBeamletHitPowerFraction_;
    }
    scalar lastFirstHitMinDistance() const { return lastFirstHitMinDistance_; }
    scalar lastFirstHitMaxDistance() const { return lastFirstHitMaxDistance_; }
    bool lastMultiRayRetraced() const { return lastMultiRayRetraced_; }
    label lastMultiRayCacheAge() const { return lastMultiRayCacheAge_; }

    void updateDeposition
    (
        const volScalarField& alphaMetal,
        const volVectorField& nFiltered
    );
};

} // End namespace Foam

#endif
