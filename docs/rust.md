# Rust

`rustc` with two tier-3 targets, `m6809-none-elf` and `m6800-none-elf`, is
built from rust-lang/rust with the patches in `rust/patches/`. Both are
`no_std`: `core` and `alloc` build, and programs link with clang against the
C runtime.

The Rust compiler is not yet built by the flake: follow
[`rust/README.md`](../rust/README.md) to build it from a dev checkout. When
it is there, `nix run .#compiler-explorer` finds it and offers Rust as well
as C and C++.
