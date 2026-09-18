/*---------------------------------------------------------------------------*\
    Parallel Lagrangian beamlet used only to locate the first metal hit.
    There is deliberately no Fresnel reflection or laser optical physics here.
\*---------------------------------------------------------------------------*/

#include "electronFirstHitParticle.H"
#include "IOstreams.H"

namespace Foam
{
    defineTypeNameAndDebug(electronFirstHitParticle, 0);

    defineTemplateTypeNameAndDebugWithName
    (
        Cloud<electronFirstHitParticle>,
        "electronFirstHitParticleCloud",
        0
    );
}


Foam::electronFirstHitParticle::electronFirstHitParticle
(
    const polyMesh& mesh,
    const point& position,
    const label cellI,
    const vector& direction,
    const label globalRayIndex
)
:
    particle(mesh, position, cellI),
    direction_(direction/(mag(direction) + VSMALL)),
    startPosition_(position),
    globalRayIndex_(globalRayIndex),
    trackHops_(0),
    active_(true)
{}


Foam::electronFirstHitParticle::electronFirstHitParticle
(
    const polyMesh& mesh,
    Istream& is,
    bool readFields,
    bool newFormat
)
:
    particle(mesh, is, readFields, newFormat),
    direction_(vector::zero),
    startPosition_(point::zero),
    globalRayIndex_(-1),
    trackHops_(0),
    active_(true)
{
    if (readFields)
    {
        is  >> direction_
            >> startPosition_
            >> globalRayIndex_
            >> trackHops_
            >> active_;
    }

    is.check(FUNCTION_NAME);
}


bool Foam::electronFirstHitParticle::move
(
    Cloud<electronFirstHitParticle>& cloud,
    trackingData& td,
    const scalar trackTime
)
{
    const polyMesh& mesh = this->mesh();
    const scalarField& alphaI = td.alphaMetal_.primitiveField();

    td.keepParticle = true;
    td.switchProcessor = false;

    const scalar maxTrackLength = mesh.bounds().mag();

    while (td.keepParticle && !td.switchProcessor && active_)
    {
        const label cellI = cell();

        if (cellI < 0 || cellI >= mesh.nCells())
        {
            active_ = false;
            td.keepParticle = false;
            break;
        }

        if (alphaI[cellI] >= td.alphaCutoff_)
        {
            const point hitPosition = position();
            const scalar hitDistance = max
            (
                (hitPosition - startPosition_) & direction_,
                scalar(0)
            );

            td.hitRayIDs_.append(globalRayIndex_);
            td.hitPositions_.append(hitPosition);
            td.hitDistances_.append(hitDistance);

            active_ = false;
            td.keepParticle = false;
            break;
        }

        if (++trackHops_ > td.maxTrackHops_)
        {
            ++td.hopLimitTerminations_;
            active_ = false;
            td.keepParticle = false;
            break;
        }

        // Rays are instantaneous. Reset before every face crossing so a ray
        // can traverse many cells and processor patches in a single move().
        stepFraction() = 0;
        const scalar f = 1.0;
        const vector displacement = direction_*maxTrackLength;

        trackToAndHitFace(displacement, f, cloud, td);
    }

    return td.keepParticle;
}


bool Foam::electronFirstHitParticle::hitPatch
(
    Cloud<electronFirstHitParticle>& cloud,
    trackingData& td
)
{
    return false;
}


void Foam::electronFirstHitParticle::hitProcessorPatch
(
    Cloud<electronFirstHitParticle>& cloud,
    trackingData& td
)
{
    td.switchProcessor = true;
}


void Foam::electronFirstHitParticle::hitWallPatch
(
    Cloud<electronFirstHitParticle>& cloud,
    trackingData& td
)
{
    active_ = false;
    td.keepParticle = false;
}


Foam::Ostream& Foam::operator<<
(
    Ostream& os,
    const electronFirstHitParticle& p
)
{
    os  << static_cast<const particle&>(p)
        << token::SPACE << p.direction_
        << token::SPACE << p.startPosition_
        << token::SPACE << p.globalRayIndex_
        << token::SPACE << p.trackHops_
        << token::SPACE << p.active_;

    os.check(FUNCTION_NAME);
    return os;
}

// ************************************************************************* //
