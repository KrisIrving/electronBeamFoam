# Calibration mesh cost reduction

The first Zakirov2020 900 W preflight showed that the original uniform-y base
mesh grows to more than two million cells after dynamic interface refinement.

A candidate three-block y mesh reduces the base count by 43.75% while
preserving the 6.25 um base depth spacing in a 0.20 mm-thick band surrounding
the free surface and top 150 um of substrate.

Profiles:

- uniformFine: 176 x 96 x 60 = 1,013,760 base cells.
- gradedY:
  - 0-0.15 mm: 12 y cells (12.5 um);
  - 0.15-0.35 mm: 32 y cells (6.25 um);
  - 0.35-0.60 mm: 10 y cells (25 um);
  - total 176 x 54 x 60 = 570,240 base cells.

The graded mesh is selected only with `MESH_PROFILE=gradedY`. The reference
uniform mesh remains the default until the 0.5 mm A/B comparison demonstrates
that fusion-zone width/depth are not materially changed.
