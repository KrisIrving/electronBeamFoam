// Evaporation mass-loss diagnostics.
//
// electronBeamFoam currently applies evaporation as an interfacial latent-heat
// sink Qv and recoil pressure, but it does not remove metal mass from alpha.
// This diagnostic converts the *already applied* evaporation cooling into the
// corresponding implied mass flux:
//
//     mDot'' = Qv / Lv
//
// and uses the same diffuse-interface weighting as TEqn:
//
//     delta_eff = |grad(alpha.metal)| * thermalDamper
//
// so that the integrated mass rate is thermodynamically consistent with the
// current energy sink.  No governing equation is changed here.
//
// cumulativeEvaporatedMassDensity is a registered AUTO_WRITE field so restart
// and AMR mapping preserve the diagnostic history.

if (evaporationDiagnosticsEnabled)
{
    const volScalarField geometricInterfaceDelta
    (
        IOobject
        (
            "geometricInterfaceDelta",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mag(gradAlpha)
    );

    const volScalarField appliedInterfaceDelta
    (
        IOobject
        (
            "appliedInterfaceDelta",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        geometricInterfaceDelta*thermalDamper
    );

    const volScalarField evaporationMassFlux
    (
        IOobject
        (
            "evaporationMassFlux",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        Qv/LatentHeatVap
    );

    const volScalarField evaporationMassSource
    (
        IOobject
        (
            "evaporationMassSource",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        evaporationMassFlux*appliedInterfaceDelta
    );

    const dimensionedScalar evaporationDeltaT
    (
        "evaporationDeltaT",
        dimTime,
        runTime.deltaTValue()
    );

    cumulativeEvaporatedMassDensity +=
        evaporationDeltaT*evaporationMassSource;
    cumulativeEvaporatedMassDensity.correctBoundaryConditions();

    if (runTime.writeTime())
    {
        const scalar evaporationMassRate =
            gSum(evaporationMassSource.primitiveField()*mesh.V());

        const scalar cumulativeEvaporatedMass =
            gSum
            (
                cumulativeEvaporatedMassDensity.primitiveField()
               *mesh.V()
            );

        const scalar maxEvaporationMassFlux =
            gMax(evaporationMassFlux.primitiveField());

        const scalar geometricInterfaceArea =
            gSum(geometricInterfaceDelta.primitiveField()*mesh.V());

        const scalar evaporationPower =
            evaporationMassRate*LatentHeatVap.value();

        const scalar equivalentEvaporatedMetalVolume =
            cumulativeEvaporatedMass/(rho1.value() + VSMALL);

        const scalar maxLocalRecessionSpeed =
            maxEvaporationMassFlux/(rho1.value() + VSMALL);

        if (Pstream::master())
        {
            Info<< "Evaporation diagnostics" << nl
                << "    mass rate             = "
                << evaporationMassRate << " kg/s" << nl
                << "    cumulative mass       = "
                << cumulativeEvaporatedMass << " kg" << nl
                << "    equivalent volume     = "
                << equivalentEvaporatedMetalVolume << " m3" << nl
                << "    evaporation power     = "
                << evaporationPower << " W" << nl
                << "    max mass flux         = "
                << maxEvaporationMassFlux << " kg/m2/s" << nl
                << "    max recession speed   = "
                << maxLocalRecessionSpeed << " m/s" << nl
                << "    geometric interface A = "
                << geometricInterfaceArea << " m2"
                << endl;

            if (evaporationDiagnosticsWriteCsv)
            {
                const fileName outputDir
                (
                    "postProcessing/evaporationDiagnostics"
                );

                mkDir(outputDir);

                const fileName csvPath
                (
                    outputDir/evaporationDiagnosticsCsvFile
                );

                const bool writeHeader = !isFile(csvPath);

                std::ofstream csv
                (
                    csvPath.c_str(),
                    std::ios::out | std::ios::app
                );

                if (!csv.good())
                {
                    WarningInFunction
                        << "Cannot open evaporation diagnostics file "
                        << csvPath << endl;
                }
                else
                {
                    csv << std::setprecision(12);

                    if (writeHeader)
                    {
                        csv
                            << "time_s,massRate_kg_per_s,"
                            << "cumulativeMass_kg,"
                            << "equivalentMetalVolume_m3,"
                            << "evaporationPower_W,"
                            << "maxMassFlux_kg_per_m2_s,"
                            << "maxRecession_m_per_s,"
                            << "geometricInterfaceArea_m2\n";
                    }

                    csv
                        << runTime.value() << ","
                        << evaporationMassRate << ","
                        << cumulativeEvaporatedMass << ","
                        << equivalentEvaporatedMetalVolume << ","
                        << evaporationPower << ","
                        << maxEvaporationMassFlux << ","
                        << maxLocalRecessionSpeed << ","
                        << geometricInterfaceArea << "\n";
                }
            }
        }
    }
}
