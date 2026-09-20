# Fusion-zone history for experimental calibration

The instantaneous liquid melt pool is not the same quantity measured from a
post-mortem metallographic cross section. Experimental single-track width/depth
normally describe the fusion boundary: material that melted at any time and
subsequently resolidified.

electronBeamFoam therefore now tracks two persistent fields:

- `peakTemperature`: maximum temperature reached in each Eulerian cell;
- `everMelted`: irreversible marker set when metallic material reaches the
  Ti-alloy liquidus temperature.

Both fields are registered OpenFOAM volume fields and are mapped during
`dynamicRefineFvMesh` topology changes. They are also restartable.

At write times, `fusionZoneDiagnostics.H` measures cumulative fusion-zone
length, width, depth and volume. For bare-plate calibration,
`fusionZoneSubstrateOnly true` excludes material above the original
free-surface plane so free-surface uplift does not inflate the metallographic
fusion boundary.

Results are written to:

```text
postProcessing/meltPoolDiagnostics/fusionZone.csv
```

This output, rather than the instantaneous `meltPool.csv`, is the primary
quantity for comparison against experimental cross-section width and depth.

The implementation is spatial/Eulerian history. It is appropriate for a
bare-plate fusion boundary, but it should not be interpreted as a material
particle history in a powder/deposition region. Powder-bed calibration will
require a separate treatment of the deposited layer.

## Log volume

`thermalCorrectorVerbose false` is now the tutorial default. It suppresses the
per-corrector epsilon1 residual line while retaining the thermal-corrector counts
in the write-time performance profile. `SummarizeRun` remains the preferred
artifact for reviewing long production runs.
