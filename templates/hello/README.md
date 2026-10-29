# Hello for the 6809 and HD6301

```sh
nix develop
make run               # build for the 6809 and run it in m6809-sim
make run TARGET=m6800  # the same for the HD6301
```

The programs link against the simulators' memory maps
(`--config=<target>-sim.cfg`). For a real board, write a linker script for
its memory map and pass it with `-T` in place of the config.
