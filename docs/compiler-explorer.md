# Compiler Explorer

```sh
nix run github:murdoa/llvm-m680x-toolchain#compiler-explorer
```

then open <http://localhost:10240>. It is a local
[Compiler Explorer](https://github.com/compiler-explorer/compiler-explorer)
with only these compilers configured: clang for C and C++ on the 6809,
HD6309 and HD6301, and `llc` for LLVM IR. "Execute" runs the program in the
simulators. Extra arguments go to Compiler Explorer, such as `--port 8080`.

Rust appears too when a `rustc` with the targets is available: set
`M6809_RUSTC`, or build it in `third_party/rust` of a checkout (see
[rust.md](rust.md)) and run from there.
