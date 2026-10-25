# Rust on the 6809 and 6800

`rustc` with two tier-3 targets, `m6809-none-elf` and `m6800-none-elf`, on
the LLVM backends in `llvm-project`. Both are `no_std`: `core` and `alloc`
build, and code links with clang against the C runtime in `runtime/`.

| | m6809-none-elf | m6800-none-elf |
|---|---|---|
| CPU | 6809 (`-C target-cpu=6309` for the HD6309) | HD6301 |
| pointer, `usize`, `c_int` | 16 bits | 16 bits |
| byte order | big-endian | big-endian |
| atomics | none (single-threaded) | none (single-threaded) |
| panics | abort | abort |

The C calling convention is clang's for these targets (see the ABI section
of `llvm-project/llvm/lib/Target/M6809/README.md`): aggregates are passed by
value on the stack and returned through a hidden pointer, and small integers
carry the same sign and zero extension attributes as in C.

## Building

1. LLVM for rustc, with the host target and both backends:

       cmake -G Ninja -S llvm-project/llvm -B build-rust \
         -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON \
         -DLLVM_TARGETS_TO_BUILD=AArch64 \
         -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="M6809;M6800" \
         -DLLVM_ENABLE_ZLIB=OFF -DLLVM_ENABLE_ZSTD=OFF -DLLVM_ENABLE_LIBXML2=OFF
       ninja -C build-rust llvm-config llvm-libraries

   Use your host's target in place of AArch64. `build/` must also have
   clang, lld and the runtime (`make -C runtime TARGET=... install`).

2. rustc, from a checkout of rust-lang/rust in `third_party/rust`:

       rust/setup.sh
       cd third_party/rust
       ./x build library --stage 1 --target m6809-none-elf,m6800-none-elf

   `setup.sh` applies `patches/` at the commit they were made on and writes
   `bootstrap.toml`. After rebuilding `build-rust`, touch
   `compiler/rustc_llvm/build.rs` so rustc links the new libraries.

## Tests

`tests/run.sh` builds each `tests/*.rs` as a static library for the 6809,
HD6309 and HD6301 at `-O0`, `-O2` and `-Oz`, links it with `-Wl,--gc-sections` and
runs it on the simulator. `main` returns 0 on success.

## Patches

- `0001`, `0002`: the targets, the `Arch` variants and the calling
  convention.
- `0003`: `core::ffi::VaArgSafe` for `i16` and `u16`, as `int` is 16 bits.
- `0004`: compiler-builtins took the shift amount of `__ashlsi3` and the
  other si3 and ti3 shifts as `u32`. LLVM passes a C `int`, which is 16 bits
  here.
- `0005` and `cc-1.2.62-m6809.diff`: bootstrap, whose pinned `cc` and
  stage0 compiler do not know the targets yet.
