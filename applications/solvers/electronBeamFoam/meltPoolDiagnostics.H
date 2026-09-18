// Automatic quantitative melt-pool diagnostics.
//
// A melt-pool cell is defined by BOTH:
//   alpha.metal >= metalFractionThreshold
//   epsilon1    >= liquidFractionThreshold
//
// Length/width are projected bounding-box extents. Depth is the maximum
// positive projection from referenceSurfacePoint along depthDirection.
// A half-cell correction based on cbrt(V) reduces the centre-sampling bias on
// the dynamically refined mesh.

if (meltPoolDiagnosticsEnabled && runTime.writeTime())
{
    const vectorField& cellCentres = mesh.C();
    const scalarField& cellVolumes = mesh.V();
    const scalarField& alphaMetalI = alpha1.primitiveField();
    const scalarField& liquidFractionI = epsilon1.primitiveField();
    const scalarField& temperatureI = T.primitiveField();
    const vectorField& velocityI = U.primitiveField();
    const scalarField& recoilPressureI = pVap.primitiveField();

    scalar minLength = GREAT;
    scalar maxLength = -GREAT;
    scalar minWidth = GREAT;
    scalar maxWidth = -GREAT;
    scalar maxDepth = -GREAT;

    scalar meltVolume = 0.0;
    scalar maxTemperature = -GREAT;
    scalar maxVelocity = 0.0;
    scalar maxRecoilPressure = 0.0;
    label meltCellCount = 0;

    forAll(cellCentres, celli)
    {
        if
        (
            alphaMetalI[celli] < meltPoolMetalFractionThreshold
         || liquidFractionI[celli] < meltPoolLiquidFractionThreshold
        )
        {
            continue;
        }

        ++meltCellCount;

        const scalar halfCell =
            0.5*cbrt(max(cellVolumes[celli], scalar(VSMALL)));

        const vector rel =
            cellCentres[celli] - meltPoolReferenceSurfacePoint;

        const scalar sLength = rel & meltPoolLengthDirection;
        const scalar sWidth = rel & meltPoolWidthDirection;
        const scalar sDepth = rel & meltPoolDepthDirection;

        minLength = min(minLength, sLength - halfCell);
        maxLength = max(maxLength, sLength + halfCell);
        minWidth = min(minWidth, sWidth - halfCell);
        maxWidth = max(maxWidth, sWidth + halfCell);
        maxDepth = max(maxDepth, sDepth + halfCell);

        // Volume of metal represented by cells passing the liquid threshold.
        meltVolume += cellVolumes[celli]*alphaMetalI[celli];

        maxTemperature = max(maxTemperature, temperatureI[celli]);
        maxVelocity = max(maxVelocity, mag(velocityI[celli]));
        maxRecoilPressure =
            max(maxRecoilPressure, recoilPressureI[celli]);
    }

    reduce(minLength, minOp<scalar>());
    reduce(maxLength, maxOp<scalar>());
    reduce(minWidth, minOp<scalar>());
    reduce(maxWidth, maxOp<scalar>());
    reduce(maxDepth, maxOp<scalar>());

    reduce(meltVolume, sumOp<scalar>());
    reduce(maxTemperature, maxOp<scalar>());
    reduce(maxVelocity, maxOp<scalar>());
    reduce(maxRecoilPressure, maxOp<scalar>());
    reduce(meltCellCount, sumOp<label>());

    scalar meltLength = 0.0;
    scalar meltWidth = 0.0;
    scalar meltDepth = 0.0;

    if (meltCellCount > 0)
    {
        meltLength = max(maxLength - minLength, scalar(0));
        meltWidth = max(maxWidth - minWidth, scalar(0));
        meltDepth = max(maxDepth, scalar(0));
    }
    else
    {
        maxTemperature = 0.0;
        maxVelocity = 0.0;
        maxRecoilPressure = 0.0;
    }

    if (Pstream::master())
    {
        Info<< "Melt-pool diagnostics" << nl
            << "    cells                 = " << meltCellCount << nl
            << "    length                = " << meltLength << " m" << nl
            << "    width                 = " << meltWidth << " m" << nl
            << "    depth                 = " << meltDepth << " m" << nl
            << "    volume                = " << meltVolume << " m3" << nl
            << "    Tmax                  = " << maxTemperature << " K" << nl
            << "    Umax                  = " << maxVelocity << " m/s" << nl
            << "    recoil max            = " << maxRecoilPressure << " Pa"
            << endl;

        if (meltPoolWriteCsv)
        {
            const fileName outputDir
            (
                "postProcessing/meltPoolDiagnostics"
            );

            mkDir(outputDir);

            const fileName csvPath(outputDir/meltPoolCsvFile);
            const bool writeHeader = !isFile(csvPath);

            std::ofstream csv
            (
                csvPath.c_str(),
                std::ios::out | std::ios::app
            );

            if (!csv.good())
            {
                WarningInFunction
                    << "Cannot open melt-pool diagnostics file "
                    << csvPath << endl;
            }
            else
            {
                csv << std::setprecision(12);

                if (writeHeader)
                {
                    csv
                        << "time_s,meltCells,length_m,width_m,depth_m,"
                        << "volume_m3,Tmax_K,Umax_m_per_s,recoilMax_Pa\n";
                }

                csv
                    << runTime.value() << ","
                    << meltCellCount << ","
                    << meltLength << ","
                    << meltWidth << ","
                    << meltDepth << ","
                    << meltVolume << ","
                    << maxTemperature << ","
                    << maxVelocity << ","
                    << maxRecoilPressure << "\n";
            }
        }
    }
}
