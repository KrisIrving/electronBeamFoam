// Cumulative fusion-zone diagnostics for comparison with post-mortem
// metallographic melt-pool cross sections.
//
// Unlike meltPoolDiagnostics.H (instantaneous liquid region), this uses the
// irreversible everMelted field. With fusionZoneSubstrateOnly=true, only the
// material at or below the original free-surface plane is included, which is
// the appropriate bare-plate fusion boundary for width/depth calibration.

if (fusionZoneEnabled && runTime.writeTime())
{
    const vectorField& cellCentres = mesh.C();
    const scalarField& cellVolumes = mesh.V();
    const scalarField& alphaMetalI = alpha1.primitiveField();
    const scalarField& everMeltedI = everMelted.primitiveField();
    const scalarField& peakTI = peakTemperature.primitiveField();

    scalar minLength = GREAT;
    scalar maxLength = -GREAT;
    scalar minWidth = GREAT;
    scalar maxWidth = -GREAT;
    scalar maxDepth = -GREAT;

    scalar fusionVolume = 0.0;
    scalar peakTemperatureMax = 0.0;
    label fusionCellCount = 0;

    forAll(cellCentres, celli)
    {
        if
        (
            everMeltedI[celli] < 0.5
         || alphaMetalI[celli] < fusionZoneMetalFractionThreshold
        )
        {
            continue;
        }

        const scalar halfCell =
            0.5*Foam::cbrt(max(cellVolumes[celli], scalar(VSMALL)));

        const vector rel =
            cellCentres[celli] - meltPoolReferenceSurfacePoint;

        const scalar sLength = rel & meltPoolLengthDirection;
        const scalar sWidth = rel & meltPoolWidthDirection;
        const scalar sDepth = rel & meltPoolDepthDirection;

        if (fusionZoneSubstrateOnly && sDepth < -halfCell)
        {
            continue;
        }

        ++fusionCellCount;

        minLength = min(minLength, sLength - halfCell);
        maxLength = max(maxLength, sLength + halfCell);
        minWidth = min(minWidth, sWidth - halfCell);
        maxWidth = max(maxWidth, sWidth + halfCell);
        maxDepth = max(maxDepth, sDepth + halfCell);

        fusionVolume += cellVolumes[celli]*alphaMetalI[celli];
        peakTemperatureMax =
            max(peakTemperatureMax, peakTI[celli]);
    }

    reduce(minLength, minOp<scalar>());
    reduce(maxLength, maxOp<scalar>());
    reduce(minWidth, minOp<scalar>());
    reduce(maxWidth, maxOp<scalar>());
    reduce(maxDepth, maxOp<scalar>());
    reduce(fusionVolume, sumOp<scalar>());
    reduce(peakTemperatureMax, maxOp<scalar>());
    reduce(fusionCellCount, sumOp<label>());

    scalar fusionLength = 0.0;
    scalar fusionWidth = 0.0;
    scalar fusionDepth = 0.0;

    if (fusionCellCount > 0)
    {
        fusionLength = max(maxLength - minLength, scalar(0));
        fusionWidth = max(maxWidth - minWidth, scalar(0));
        fusionDepth = max(maxDepth, scalar(0));
    }

    if (Pstream::master())
    {
        Info<< "Fusion-zone diagnostics" << nl
            << "    cells                 = " << fusionCellCount << nl
            << "    length                = " << fusionLength << " m" << nl
            << "    width                 = " << fusionWidth << " m" << nl
            << "    depth                 = " << fusionDepth << " m" << nl
            << "    volume                = " << fusionVolume << " m3" << nl
            << "    peak Tmax             = " << peakTemperatureMax << " K"
            << endl;

        if (fusionZoneWriteCsv)
        {
            const fileName outputDir
            (
                "postProcessing/meltPoolDiagnostics"
            );

            mkDir(outputDir);

            const fileName csvPath(outputDir/fusionZoneCsvFile);
            const bool writeHeader = !isFile(csvPath);

            std::ofstream csv
            (
                csvPath.c_str(),
                std::ios::out | std::ios::app
            );

            if (!csv.good())
            {
                WarningInFunction
                    << "Cannot open fusion-zone diagnostics file "
                    << csvPath << endl;
            }
            else
            {
                csv << std::setprecision(12);

                if (writeHeader)
                {
                    csv
                        << "time_s,fusionCells,length_m,width_m,depth_m,"
                        << "volume_m3,peakTmax_K\n";
                }

                csv
                    << runTime.value() << ","
                    << fusionCellCount << ","
                    << fusionLength << ","
                    << fusionWidth << ","
                    << fusionDepth << ","
                    << fusionVolume << ","
                    << peakTemperatureMax << "\n";
            }
        }
    }
}
