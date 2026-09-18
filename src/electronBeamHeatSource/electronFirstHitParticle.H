/*---------------------------------------------------------------------------*\
License
    This file is part of electronBeamFoam, derived from laserbeamFoam.
\*---------------------------------------------------------------------------*/

#ifndef electronFirstHitParticle_H
#define electronFirstHitParticle_H

#include "particle.H"
#include "Cloud.H"
#include "volFields.H"
#include "DynamicList.H"
#include "IOstream.H"

namespace Foam
{

class electronFirstHitParticle;
typedef Cloud<electronFirstHitParticle> electronFirstHitParticleCloud;

Ostream& operator<<(Ostream&, const electronFirstHitParticle&);

class electronFirstHitParticle
:
    public particle
{
public:

    vector direction_;
    point startPosition_;
    label globalRayIndex_;
    label trackHops_;
    bool active_;

    class trackingData
    :
        public particle::trackingData
    {
    public:

        const volScalarField& alphaMetal_;
        scalar alphaCutoff_;
        label maxTrackHops_;

        DynamicList<label>& hitRayIDs_;
        DynamicList<point>& hitPositions_;
        DynamicList<scalar>& hitDistances_;

        label& hopLimitTerminations_;

        trackingData
        (
            const Cloud<electronFirstHitParticle>& cloud,
            const volScalarField& alphaMetal,
            const scalar alphaCutoff,
            const label maxTrackHops,
            DynamicList<label>& hitRayIDs,
            DynamicList<point>& hitPositions,
            DynamicList<scalar>& hitDistances,
            label& hopLimitTerminations
        )
        :
            particle::trackingData(cloud),
            alphaMetal_(alphaMetal),
            alphaCutoff_(alphaCutoff),
            maxTrackHops_(maxTrackHops),
            hitRayIDs_(hitRayIDs),
            hitPositions_(hitPositions),
            hitDistances_(hitDistances),
            hopLimitTerminations_(hopLimitTerminations)
        {}
    };

    TypeName("electronFirstHitParticle");

    electronFirstHitParticle
    (
        const polyMesh& mesh,
        const point& position,
        const label cellI,
        const vector& direction,
        const label globalRayIndex
    );

    electronFirstHitParticle
    (
        const polyMesh& mesh,
        Istream& is,
        bool readFields = true,
        bool newFormat = true
    );

    autoPtr<particle> clone() const
    {
        return autoPtr<particle>(new electronFirstHitParticle(*this));
    }

    class iNew
    {
        const polyMesh& mesh_;

    public:

        iNew(const polyMesh& mesh)
        :
            mesh_(mesh)
        {}

        autoPtr<electronFirstHitParticle> operator()(Istream& is) const
        {
            return autoPtr<electronFirstHitParticle>
            (
                new electronFirstHitParticle(mesh_, is, true)
            );
        }
    };

    virtual ~electronFirstHitParticle() = default;

    bool move
    (
        Cloud<electronFirstHitParticle>& cloud,
        trackingData& td,
        const scalar trackTime
    );

    bool hitPatch
    (
        Cloud<electronFirstHitParticle>& cloud,
        trackingData& td
    );

    void hitProcessorPatch
    (
        Cloud<electronFirstHitParticle>& cloud,
        trackingData& td
    );

    void hitWallPatch
    (
        Cloud<electronFirstHitParticle>& cloud,
        trackingData& td
    );

    friend Ostream& operator<<
    (
        Ostream& os,
        const electronFirstHitParticle& p
    );
};

} // End namespace Foam

#endif

// ************************************************************************* //
