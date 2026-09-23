// Station-wise cumulative fusion-zone diagnostics.
//
// Each requested station is expressed in metres along lengthDirection relative
// to referenceSurfacePoint. A finite slab centered on the station is sampled:
//     |sLength - station| <= fusionZoneSectionHalfThickness
//
// The slab formulation is intentional. It remains robust on a dynamically
// refined mesh and avoids pretending that a cell-centred VOF/thermal history
// field provides an exact mathematical plane intersection. Width/depth use
// the same half-cell correction as the global fusion-zone diagnostic.
//
// Output:
// time_s,station_m,sectionCells,width_m,depth_m,peakTmax_K
//
// The final rows at central-track stations are the primary observables for
// comparison with metallographic y-z cross sections. Global fusionZone.csv is
// retained as a secondary whole-track envelope diagnostic.

if
(
    fusionZoneEnabled
 && fusionZoneSectionEnabled
 && runTime.writeTime()
)
{
    const vectorField& sectionCellCentres = mesh.C();
    const scalarField& sectionCellVolumes = mesh.V();
    const scalarField& sectionAlphaMetal = alpha1.primitiveField();
    const scalarField& sectionEverMelted = everMelted.primitiveField();
    const scalarField& sectionPeakT = peakTemperature.primitiveField();

    forAll(fusionZoneSectionPositions, stationi)
    {
        const scalar station = fusionZoneSectionPositions[stationi];

        scalar minSectionWidth = GREAT;
        scalar maxSectionWidth = -GREAT;
        scalar maxSectionDepth = -GREAT;
        scalar sectionPeakTemperature = 0.0;
        label sectionCellCount = 0;

        forAll(sectionCellCentres, celli)
        {
            if
            (
                sectionEverMelted[celli] < 0.5
             || sectionAlphaMetal[celli] < fusionZoneMetalFractionThreshold
            )
            {
                continue;
            }

            const vector rel =
                sectionCellCentres[celli] - meltPoolReferenceSurfacePoint;

            const scalar sLength = rel & meltPoolLengthDirection;

            if
            (
                Foam::mag(sLength - station)
              > fusionZoneSectionHalfThickness + SMALL
            )
            {
                continue;
            }

            const scalar halfCell =
                0.5*Foam::cbrt
                (
                    max(sectionCellVolumes[celli], scalar(VSMALL))
                );

            const scalar sWidth = rel & meltPoolWidthDirection;
            const scalar sDepth = rel & meltPoolDepthDirection;

            if (fusionZoneSubstrateOnly && sDepth < -halfCell)
            {
                continue;
            }

            ++sectionCellCount;

            minSectionWidth =
                min(minSectionWidth, sWidth - halfCell);
            maxSectionWidth =
                max(maxSectionWidth, sWidth + halfCell);
            maxSectionDepth =
                max(maxSectionDepth, sDepth + halfCell);

            sectionPeakTemperature =
                max(sectionPeakTemperature, sectionPeakT[celli]);
        }

        reduce(minSectionWidth, minOp<scalar>());
        reduce(maxSectionWidth, maxOp<scalar>());
        reduce(maxSectionDepth, maxOp<scalar>());
        reduce(sectionPeakTemperature, maxOp<scalar>());
        reduce(sectionCellCount, sumOp<label>());

        scalar sectionWidth = 0.0;
        scalar sectionDepth = 0.0;

        if (sectionCellCount > 0)
        {
            sectionWidth =
                max(maxSectionWidth - minSectionWidth, scalar(0));
            sectionDepth = max(maxSectionDepth, scalar(0));
        }

        if (Pstream::master())
        {
            Info<< "Fusion-zone section diagnostics" << nl
                << "    station               = " << station << " m" << nl
                << "    slab half-thickness   = "
                << fusionZoneSectionHalfThickness << " m" << nl
                << "    cells                 = " << sectionCellCount << nl
                << "    width                 = " << sectionWidth << " m" << nl
                << "    depth                 = " << sectionDepth << " m" << nl
                << "    peak Tmax             = "
                << sectionPeakTemperature << " K"
                << endl;

            const fileName outputDir
            (
                "postProcessing/meltPoolDiagnostics"
            );

            mkDir(outputDir);

            const fileName csvPath
            (
                outputDir/fusionZoneSectionCsvFile
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
                    << "Cannot open fusion-zone section diagnostics file "
                    << csvPath << endl;
            }
            else
            {
                csv << std::setprecision(12);

                if (writeHeader)
                {
                    csv
                        << "time_s,station_m,sectionCells,width_m,depth_m,"
                        << "peakTmax_K\n";
                }

                csv
                    << runTime.value() << ","
                    << station << ","
                    << sectionCellCount << ","
                    << sectionWidth << ","
                    << sectionDepth << ","
                    << sectionPeakTemperature << "\n";
            }
        }
    }
}
