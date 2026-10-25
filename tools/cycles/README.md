# Cycle comparison

`run.py` builds the workloads in `workloads.c` (built on the size
benchmarks in `tools/size/bench`) for the 6809 and for `-mcpu=6309`, and runs
each on the matching CPU in `tools/m6809-sim`. The HD6309 stays in 6809
emulation mode, so it uses emulation-mode cycle counts.

    python3 tools/cycles/run.py [--opt 2] [workload ...]

## Status

At `-O2`, HD6309 code takes 0.66 of the 6809 cycles in total (2374505
against 3594038). Most of the gain is in `checksum` and `convert`, where
DIVQ replaces the library divide loop for `% 255` and `/ 10`.
