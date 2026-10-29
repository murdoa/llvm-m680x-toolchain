# llvm-m680x-toolchain

C, C++ and Rust for the Motorola 6800 and 6809 family, on LLVM: clang,
`ld.lld`, a runtime and simulators for the **6809**, the **HD6309** and the
**HD6301/HD6303**, packaged with Nix.

```sh
cat > hello.c <<'C'
#include <stdio.h>
int main(void) { printf("Hello, 6809\n"); return 0; }
C
nix run github:murdoa/llvm-m680x-toolchain#clang -- \
  --target=m6809 --config=m6809-sim.cfg -O2 hello.c -o hello.elf
nix run github:murdoa/llvm-m680x-toolchain#m6809-sim -- hello.elf
```

There is no binary cache yet, so the first run builds LLVM from source,
which takes a long time.

## Status

Experimental. The backends live on a fork of LLVM
([murdoa/llvm-project](https://github.com/murdoa/llvm-project), branch
`m6809`) and are not upstream.

- Every applicable GCC c-torture execute test passes at `-O0`, `-O2` and
  `-Oz` on the 6809, the HD6309 and the HD6301, with the machine verifier
  on.
- At `-Oz`, 6809 code is 0.97 of CMOC's size, and HD6309 code 0.92.
- C++20 and `no_std` Rust (`core`, `alloc`) build and run on all three.
- It builds the firmware of a Solartron 7081 voltmeter, which passes the
  instrument's cycle-sensitive acceptance tests.

[docs/goals.md](docs/goals.md) has the milestones and what is not planned.

## Targets

| | `--target` | `-mcpu` |
|---|---|---|
| Motorola 6809 | `m6809` | `6809` (default) |
| Hitachi HD6309 | `m6809` | `6309` |
| Hitachi HD6301/HD6303 | `m6800` | `6301` (default) |

See [docs/targets.md](docs/targets.md) for the types, the calling
convention, interrupt handlers and the direct page.

## Packages

| `nix build .#…` | What |
|---|---|
| `m680x-toolchain` (default) | clang, lld, the LLVM tools and the runtime for both targets |
| `llvm` | LLVM, clang and lld alone |
| `m6809-sim`, `m6800-sim` | the simulators |
| `compiler-explorer-m6809` | a local [Compiler Explorer](docs/compiler-explorer.md) |

`nix run .#clang`, `.#clang++`, `.#m6809-sim`, `.#m6800-sim` and
`.#compiler-explorer` run them.

## Use it in a project

Start from the template:

```sh
nix flake init -t github:murdoa/llvm-m680x-toolchain
nix develop
make run               # the 6809
make run TARGET=m6800  # the HD6301
```

or add the input and its overlay to your flake:

```nix
inputs.m680x.url = "github:murdoa/llvm-m680x-toolchain";
# ...
pkgs = import nixpkgs { inherit system; overlays = [ m680x.overlays.default ]; };
# pkgs.m680x-toolchain, pkgs.m6809-sim, pkgs.m6800-sim
```

For your own board, see [docs/bare-metal.md](docs/bare-metal.md): a linker
script for its memory map, `_exit`, and the character I/O hooks if you
use `stdio.h`.

## Documentation

- [Targets and ABI](docs/targets.md)
- [Bare-metal programs](docs/bare-metal.md)
- [Rust](docs/rust.md)
- [Compiler Explorer](docs/compiler-explorer.md)
- [Working on the toolchain](docs/development.md)
- [Goals and milestones](docs/goals.md)

## Licence

Apache License v2.0 with LLVM Exceptions, as LLVM: see [LICENSE](LICENSE).
