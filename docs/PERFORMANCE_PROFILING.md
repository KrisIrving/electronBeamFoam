# Runtime performance profiling

The solver contains a lightweight wall-clock profiler intended to identify the
actual bottleneck before changing numerical algorithms.

Enable it in `system/controlDict`:

```
performanceProfiling true;
```

A summary is printed only at normal write times. No MPI timing reductions are
performed on every CFD step; local timings are accumulated and reduced only
when the case already writes output.

Reported categories:

- dynamic mesh update;
- VOF/interface work excluding mesh update;
- thermophysical/property updates;
- electron-beam tracking and deposition;
- momentum equation;
- temperature/phase-change correction;
- pressure correction;
- turbulence correction;
- write I/O;
- unclassified loop overhead.

The report also includes CFD step count, PIMPLE/pressure iteration counts,
thermal phase-correction counts, time-step min/mean/max and current global cell
count.

For wall-clock categories, the reported value is the maximum accumulated time
among MPI ranks, which approximates the critical parallel path.

Use the same 100 microsecond single-track benchmark first. The next optimisation
should target the dominant measured category rather than the electron-beam
source by assumption.
