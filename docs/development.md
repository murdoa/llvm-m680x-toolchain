# Working on the toolchain

The flake builds a pinned commit of the LLVM fork
(`github:murdoa/llvm-project`, branch `m6809`). To change the backend,
work in a local clone and an incremental build instead.

## Setup

```sh
git clone -b m6809 https://github.com/murdoa/llvm-project.git
nix develop
./configure.sh                       # cmake into build/
ninja -C build clang lld llc llvm-mc llvm-objdump llvm-ar
make -C runtime install              # the 6809 runtime, into build/
make -C runtime TARGET=m6800 install
make -C tools/m6809-sim && make -C tools/m6800-sim
```

`configure.sh` builds the M6809 and M6800 targets only, with assertions.
`HOST_TARGETS`, `TARGETS` and `PROJECTS` override what it builds. Builds
are large: limit parallel jobs if memory is short.

## Tests

| What | How |
|---|---|
| lit tests | `build/bin/llvm-lit llvm-project/llvm/test/CodeGen/M6809 llvm-project/llvm/test/MC/M6809` (and `M6800`) |
| runtime tests | `make -C runtime check` (`TARGET=m6800`, or `TEST_CPU=-mcpu=6309`) |
| GCC c-torture | [`tools/torture/README.md`](../tools/torture/README.md) |
| code size against CMOC | [`tools/size/README.md`](../tools/size/README.md) |
| cycle counts | [`tools/cycles/README.md`](../tools/cycles/README.md) |
| simulators against a reference | `make -C tools/m6809-sim difftest PYCORE=<dir>`, where `<dir>` holds a Python 6809 or HD6301 core (`emu/mc6809.py`, `emu/hd6301.py`) |
| Rust | [`rust/README.md`](../rust/README.md) |
| everything the flake checks | `nix flake check -L` |

`nix flake check` builds the pinned LLVM and the runtime, runs the runtime
tests on the 6809, HD6309 and HD6301 simulators, and builds and runs the
template.

## Updating the pin

```sh
nix flake update llvm-project
nix flake check -L
```

After `nix flake update compiler-explorer`, update `npmDepsHash` in
`nix/compiler-explorer/package.nix`.

## Layout

| Path | What |
|---|---|
| `flake.nix`, `nix/` | packages, checks, apps, the overlay and Compiler Explorer |
| `runtime/` | crt0, libc, compiler-rt and libc++ builds, the simulator board support and tests |
| `tools/m6809-sim`, `tools/m6800-sim` | cycle-counting simulators that run test programs |
| `tools/torture`, `tools/size`, `tools/cycles`, `tools/statemachine` | test and measurement harnesses |
| `rust/` | patches and scripts for rustc |
| `templates/hello` | the `nix flake init` template |
| `docs/` | these documents and the project goals |
