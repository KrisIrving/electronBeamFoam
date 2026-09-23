/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2020 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    electronBeamFoam

Group
    grpMultiphaseSolvers

Description
    Electron-beam heat-source implementation with two-phase incompressible VOF
    description of the metallic substrate and a numerical vacuum/void phase,
    with optional mesh motion and mesh topology changes including adaptive
    re-meshing.
Authors

    Tom Flint, UoM.
    Philip Cardiff, UCD.
    Gowthaman Parivendhan, UCD.
    Joe Robson, UoM.
    Petar Cosic, UCD
    Simon Rodriguez, UCD

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "dynamicFvMesh.H"
#include "isoAdvection.H"
#include "CMULES.H"
#include "EulerDdtScheme.H"
#include "localEulerDdtScheme.H"
#include "CrankNicolsonDdtScheme.H"
#include "subCycle.H"
#include "immiscibleIncompressibleTwoPhaseMixture.H"
#include "incompressibleInterPhaseTransportModel.H"
#include "turbulentTransportModel.H"
#include "pimpleControl.H"
#include "fvOptions.H"
#include "CorrectPhi.H"
#include "fvcSmooth.H"
#include "dynamicRefineFvMesh.H"

#include "Polynomial.H"
#include "electronBeamHeatSource.H"
#include "mthdModel.H"

#include <chrono>
#include <fstream>
#include <iomanip>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::addNote
    (
        "Solver for two incompressible, isothermal immiscible fluids"
        " using VOF phase-fraction based interface capturing.\n"
        "With optional mesh motion and mesh topology changes including"
        " adaptive re-meshing."
    );

    #include "postProcess.H"

    #include "addCheckCaseOptions.H"
    #include "setRootCaseLists.H"
    #include "createTime.H"
    #include "createDynamicFvMesh.H"
    #include "initContinuityErrs.H"
    #include "createDyMControls.H"
    #include "createFields.H"
    #include "MULES/createAlphaFluxes.H"
    #include "initCorrectPhi.H"
    #include "createUfIfPresent.H"

    if (interfaceTrackingScheme == "MULES")
    {
        if (!LTS)
        {
            #include "MULES/CourantNo.H"
            #include "setInitialDeltaT.H"
        }
    }
    else if (interfaceTrackingScheme == "isoAdvector")
    {
        #include "isoAdvector/porousCourantNo.H"
        #include "setInitialDeltaT.H"
    }

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
    const Switch performanceProfiling
    (
        runTime.controlDict().lookupOrDefault<Switch>
        (
            "performanceProfiling",
            true
        )
    );

    const Switch thermalCorrectorVerbose
    (
        runTime.controlDict().lookupOrDefault<Switch>
        (
            "thermalCorrectorVerbose",
            false
        )
    );

    using profileClock = std::chrono::steady_clock;

    const auto profileNow = []()
    {
        return profileClock::now();
    };

    const auto profileElapsed =
    []
    (
        const profileClock::time_point& start
    ) -> scalar
    {
        return scalar
        (
            std::chrono::duration<double>
            (
                profileClock::now() - start
            ).count()
        );
    };

    scalar profileStepWall = 0.0;
    scalar profileMeshWall = 0.0;
    scalar profileAlphaWall = 0.0;
    scalar profilePropsWall = 0.0;
    scalar profileBeamWall = 0.0;
    scalar profileMomentumWall = 0.0;
    scalar profileThermalWall = 0.0;
    scalar profilePressureWall = 0.0;
    scalar profileTurbulenceWall = 0.0;
    scalar profileWriteWall = 0.0;

    label profileSteps = 0;
    label profilePimpleIterations = 0;
    label profilePressureCorrectors = 0;
    label profileThermalCorrectors = 0;
    label profileMaxThermalCorrectors = 0;
    label profileThermalCapHits = 0;
    scalar profileThermalFinalResidualSum = 0.0;
    scalar profileThermalFinalResidualMax = 0.0;
    scalar profileThermalConvergenceResidualSum = 0.0;
    scalar profileThermalConvergenceResidualMax = 0.0;

    // Region-resolved final liquid-fraction residual diagnostics.
    // These are diagnostic-only and do not alter the phase-change solve.
    scalar profileThermalBulkResidualSum = 0.0;
    scalar profileThermalBulkResidualMax = 0.0;
    scalar profileThermalInterfaceResidualSum = 0.0;
    scalar profileThermalInterfaceResidualMax = 0.0;
    scalar profileThermalVoidResidualSum = 0.0;
    scalar profileThermalVoidResidualMax = 0.0;
    label profileThermalBulkCapHits = 0;
    label profileThermalInterfaceCapHits = 0;
    label profileThermalVoidCapHits = 0;

    scalar profileDeltaTSum = 0.0;
    scalar profileDeltaTMin = GREAT;
    scalar profileDeltaTMax = 0.0;

    Info<< "electronBeamFoam build tag = fusionSections-v4-pCorr2Baseline" << nl
        << "\nStarting time loop\n" << endl;

    if (performanceProfiling)
    {
        Info<< "Performance profiling enabled; interval summaries are written "
            << "at normal output times." << nl << endl;
    }

    while (runTime.run())
    {
        const auto profileStepStart = profileNow();

        #include "readControls.H"
        #include "readDyMControls.H"

        if (interfaceTrackingScheme == "MULES")
        {
            if (LTS)
            {
                #include "MULES/setRDeltaT.H"
            }
            else
            {
                #include "MULES/CourantNo.H"
                #include "MULES/alphaCourantNo.H"
                #include "MULES/setDeltaT.H"
            }
        }
        else if (interfaceTrackingScheme == "isoAdvector")
        {
            #include "isoAdvector/porousCourantNo.H"
            #include "isoAdvector/porousAlphaCourantNo.H"
            #include "isoAdvector/setDeltaT.H"
        }

        ++runTime;

        if (performanceProfiling)
        {
            ++profileSteps;
            profileDeltaTSum += runTime.deltaTValue();
            profileDeltaTMin =
                min(profileDeltaTMin, runTime.deltaTValue());
            profileDeltaTMax =
                max(profileDeltaTMax, runTime.deltaTValue());
        }

        Info<< "Time = " << runTime.timeName() << nl << endl;

        // --- Pressure-velocity PIMPLE corrector loop
        while (pimple.loop())
        {
            if (performanceProfiling)
            {
                ++profilePimpleIterations;
            }

            const auto profileAlphaStart = profileNow();

            if (interfaceTrackingScheme == "MULES")
            {
                #include "MULES/firstIter.H"
                #include "MULES/alphaControls.H"
                #include "MULES/alphaEqnSubCycle.H"
            }
            else if (interfaceTrackingScheme == "isoAdvector")
            {
                #include "isoAdvector/firstIter.H"
                #include "isoAdvector/alphaControls.H"
                #include "isoAdvector/alphaEqnSubCycle.H"
            }

            if (performanceProfiling)
            {
                profileAlphaWall += profileElapsed(profileAlphaStart);
            }

            {
                const auto profilePropsStart = profileNow();
                #include "updateProps.H"
                if (performanceProfiling)
                {
                    profilePropsWall += profileElapsed(profilePropsStart);
                }
            }

            // Update electron-beam energy deposition field
            {
                const auto profileBeamStart = profileNow();
                electronBeam.updateDeposition(alpha_filtered, n_filtered);
                if (performanceProfiling)
                {
                    profileBeamWall += profileElapsed(profileBeamStart);
                }
            }

            {
                const auto profilePropsStart = profileNow();
                mixture.correct();
                if (performanceProfiling)
                {
                    profilePropsWall += profileElapsed(profilePropsStart);
                }
            }

            if (pimple.frozenFlow())
            {
                continue;
            }

            // UEqn must remain in the PIMPLE-loop scope because pEqn.H
            // accesses UEqn.A() and UEqn.H() later in the same iteration.
            const auto profileMomentumStart = profileNow();

            #include "UEqn.H"

            if (mthd.valid())
            {
                mthd->solve(phi, U);
            }

            if (performanceProfiling)
            {
                profileMomentumWall +=
                    profileElapsed(profileMomentumStart);
            }

            {
                const auto profileThermalStart = profileNow();

                #include "TEqn.H"

                if (performanceProfiling)
                {
                    profileThermalWall +=
                        profileElapsed(profileThermalStart);
                }
            }

            // --- Pressure corrector loop
            {
                const auto profilePressureStart = profileNow();

                while (pimple.correct())
                {
                    if (performanceProfiling)
                    {
                        ++profilePressureCorrectors;
                    }

                    #include "pEqn.H"
                }

                if (performanceProfiling)
                {
                    profilePressureWall +=
                        profileElapsed(profilePressureStart);
                }
            }

            if (pimple.turbCorr())
            {
                const auto profileTurbulenceStart = profileNow();

                turbulence->correct();

                if (performanceProfiling)
                {
                    profileTurbulenceWall +=
                        profileElapsed(profileTurbulenceStart);
                }
            }
        }

        // Update instantaneous liquid metal indicator and irreversible thermal
        // history. A cell enters everMelted only after metallic material in
        // that spatial location reaches the Ti-alloy liquidus.
        const volScalarField& alphaMetal =
            mesh.lookupObject<volScalarField>("alpha.metal");
        liquidMetalCells = pos(alphaMetal - 0.5)*pos(epsilon1 - 0.5);

        {
            scalarField& peakTI = peakTemperature.primitiveFieldRef();
            scalarField& everMeltedI = everMelted.primitiveFieldRef();
            const scalarField& TI = T.primitiveField();
            const scalarField& alphaMetalI = alphaMetal.primitiveField();
            const scalar liquidus = Tliquidus1.value();

            forAll(TI, celli)
            {
                peakTI[celli] = max(peakTI[celli], TI[celli]);

                if
                (
                    alphaMetalI[celli] >= fusionZoneMetalFractionThreshold
                 && TI[celli] >= liquidus
                )
                {
                    everMeltedI[celli] = 1.0;
                }
            }

            peakTemperature.correctBoundaryConditions();
            everMelted.correctBoundaryConditions();
        }

        const bool profileReportNow =
            performanceProfiling && runTime.writeTime();

        #include "meltPoolDiagnostics.H"
        #include "fusionZoneDiagnostics.H"
        #include "fusionZoneSectionDiagnostics.H"

        const auto profileWriteStart = profileNow();
        runTime.write();

        if (performanceProfiling)
        {
            profileWriteWall += profileElapsed(profileWriteStart);
            profileStepWall += profileElapsed(profileStepStart);
        }

        if (profileReportNow)
        {
            // Report the slowest MPI rank for every wall-clock category.
            scalar wallTotal = profileStepWall;
            scalar wallMesh = profileMeshWall;
            scalar wallAlpha = profileAlphaWall;
            scalar wallProps = profilePropsWall;
            scalar wallBeam = profileBeamWall;
            scalar wallMomentum = profileMomentumWall;
            scalar wallThermal = profileThermalWall;
            scalar wallPressure = profilePressureWall;
            scalar wallTurbulence = profileTurbulenceWall;
            scalar wallWrite = profileWriteWall;

            reduce(wallTotal, maxOp<scalar>());
            reduce(wallMesh, maxOp<scalar>());
            reduce(wallAlpha, maxOp<scalar>());
            reduce(wallProps, maxOp<scalar>());
            reduce(wallBeam, maxOp<scalar>());
            reduce(wallMomentum, maxOp<scalar>());
            reduce(wallThermal, maxOp<scalar>());
            reduce(wallPressure, maxOp<scalar>());
            reduce(wallTurbulence, maxOp<scalar>());
            reduce(wallWrite, maxOp<scalar>());

            const scalar wallVofOnly =
                max(wallAlpha - wallMesh, scalar(0));

            const scalar accounted =
                wallMesh
              + wallVofOnly
              + wallProps
              + wallBeam
              + wallMomentum
              + wallThermal
              + wallPressure
              + wallTurbulence
              + wallWrite;

            const scalar wallOther =
                max(wallTotal - accounted, scalar(0));

            const scalar invWall =
                100.0/(wallTotal + VSMALL);

            label globalCells = mesh.nCells();
            reduce(globalCells, sumOp<label>());

            const scalar meanDeltaT =
                profileDeltaTSum/max(profileSteps, label(1));

            if (Pstream::master())
            {
                Info<< "Performance profile: since previous write" << nl
                    << "    steps                = " << profileSteps << nl
                    << "    PIMPLE iterations    = "
                    << profilePimpleIterations << nl
                    << "    pressure correctors  = "
                    << profilePressureCorrectors << nl
                    << "    thermal correctors   = "
                    << profileThermalCorrectors << nl
                    << "    max thermal/TEqn     = "
                    << profileMaxThermalCorrectors << nl
                    << "    thermal cap hits     = "
                    << profileThermalCapHits << nl
                    << "    thermal final resid  = "
                    << profileThermalFinalResidualSum
                       /max(profilePimpleIterations, label(1))
                    << " mean / "
                    << profileThermalFinalResidualMax << " max" << nl
                    << "    thermal conv resid   = "
                    << profileThermalConvergenceResidualSum
                       /max(profilePimpleIterations, label(1))
                    << " mean / "
                    << profileThermalConvergenceResidualMax << " max" << nl
                    << "    thermal resid bulk   = "
                    << profileThermalBulkResidualSum
                       /max(profilePimpleIterations, label(1))
                    << " meanMax / "
                    << profileThermalBulkResidualMax << " max" << nl
                    << "    thermal resid iface  = "
                    << profileThermalInterfaceResidualSum
                       /max(profilePimpleIterations, label(1))
                    << " meanMax / "
                    << profileThermalInterfaceResidualMax << " max" << nl
                    << "    thermal resid void   = "
                    << profileThermalVoidResidualSum
                       /max(profilePimpleIterations, label(1))
                    << " meanMax / "
                    << profileThermalVoidResidualMax << " max" << nl
                    << "    thermal cap regions  = "
                    << profileThermalBulkCapHits << " bulk / "
                    << profileThermalInterfaceCapHits << " iface / "
                    << profileThermalVoidCapHits << " void" << nl
                    << "    deltaT min/mean/max  = "
                    << profileDeltaTMin << " / "
                    << meanDeltaT << " / "
                    << profileDeltaTMax << " s" << nl
                    << "    global cells         = " << globalCells << nl
                    << "    wall total           = " << wallTotal
                    << " s (100%)" << nl
                    << "    mesh update          = " << wallMesh
                    << " s (" << wallMesh*invWall << "%)" << nl
                    << "    VOF/interface        = " << wallVofOnly
                    << " s (" << wallVofOnly*invWall << "%)" << nl
                    << "    properties           = " << wallProps
                    << " s (" << wallProps*invWall << "%)" << nl
                    << "    electron beam        = " << wallBeam
                    << " s (" << wallBeam*invWall << "%)" << nl
                    << "    momentum             = " << wallMomentum
                    << " s (" << wallMomentum*invWall << "%)" << nl
                    << "    thermal/phase        = " << wallThermal
                    << " s (" << wallThermal*invWall << "%)" << nl
                    << "    pressure             = " << wallPressure
                    << " s (" << wallPressure*invWall << "%)" << nl
                    << "    turbulence           = " << wallTurbulence
                    << " s (" << wallTurbulence*invWall << "%)" << nl
                    << "    write I/O            = " << wallWrite
                    << " s (" << wallWrite*invWall << "%)" << nl
                    << "    other                = " << wallOther
                    << " s (" << wallOther*invWall << "%)" << endl;
            }

            profileStepWall = 0.0;
            profileMeshWall = 0.0;
            profileAlphaWall = 0.0;
            profilePropsWall = 0.0;
            profileBeamWall = 0.0;
            profileMomentumWall = 0.0;
            profileThermalWall = 0.0;
            profilePressureWall = 0.0;
            profileTurbulenceWall = 0.0;
            profileWriteWall = 0.0;

            profileSteps = 0;
            profilePimpleIterations = 0;
            profilePressureCorrectors = 0;
            profileThermalCorrectors = 0;
            profileMaxThermalCorrectors = 0;
            profileThermalCapHits = 0;
            profileThermalFinalResidualSum = 0.0;
            profileThermalFinalResidualMax = 0.0;
            profileThermalConvergenceResidualSum = 0.0;
            profileThermalConvergenceResidualMax = 0.0;
            profileThermalBulkResidualSum = 0.0;
            profileThermalBulkResidualMax = 0.0;
            profileThermalInterfaceResidualSum = 0.0;
            profileThermalInterfaceResidualMax = 0.0;
            profileThermalVoidResidualSum = 0.0;
            profileThermalVoidResidualMax = 0.0;
            profileThermalBulkCapHits = 0;
            profileThermalInterfaceCapHits = 0;
            profileThermalVoidCapHits = 0;

            profileDeltaTSum = 0.0;
            profileDeltaTMin = GREAT;
            profileDeltaTMax = 0.0;
        }

        runTime.printExecutionTime(Info);
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
