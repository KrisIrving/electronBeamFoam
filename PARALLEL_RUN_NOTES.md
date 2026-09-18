# Parallel run notes for the current workstation

The supplied build log shows OpenFOAM **v2512** and a successful build of
`electronBeamFoam` (no compiler/linker errors).  The workstation screenshot
shows 96 logical CPUs and 93 GiB RAM.

## Default

The electronBeamFoam tutorials now default to **32 MPI ranks** and write all
OpenFOAM output to `log.*` files instead of using `tee`.

```bash
NPROCS=32 ./Allrun_parallel
```

Detached:

```bash
NPROCS=32 ./Run_background
```

Status:

```bash
./Status
```

Full live log only when deliberately requested:

```bash
tail -f log.electronBeamFoam
```

## Why not 96 ranks by default?

The GNOME hardware panel reports logical CPUs, not necessarily 96 independent
physical cores.  For a small/medium CFD mesh, 96 MPI ranks can be slower due to
communication and can leave too few cells per rank. Start with 32 and benchmark
16/32/48 after recording the actual cell count and timestep behaviour.

## I/O changes

`writeFormat` is set to `binary` for the supplied electronBeamFoam tutorials.
This reduces output volume and usually improves write performance in parallel.

## Heat-source runtime changes

The electron-beam source no longer prints/integrates absorbed power every time
step.  The explicit global power integral is evaluated only at write times.
The Gaussian calculation also skips cells farther than four beam radii and
rejects cells outside the penetration layer before evaluating exponentials.
These changes preserve the normalised absorbed power while reducing per-step
CPU/MPI/logging overhead.
