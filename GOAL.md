# End goal

**Modern C, C++ and Rust for the Motorola 6809, the Hitachi HD6309 and the
Hitachi HD6301, through LLVM backends good enough to propose upstream and
proven on real firmware for the Solartron 7081.**

The 7081 has a 6809 main processor and an HD6301 on the floating board, so
it covers both backends. Its firmware is the practical test: if we can build
working replacements for parts of it, the toolchain works.

## Done means

Each item can be checked.

1. **One compiler, three CPUs.**
   `clang --target=m6809 -mcpu=6809`, `-mcpu=6309` and
   `clang --target=m6800 -mcpu=6301` compile freestanding C17 and C++
   (no exceptions or RTTI) at `-O0`, `-O2` and `-Oz`.
2. **Binaries without outside tools.** LLVM's integrated assembler writes
   ELF objects, and `ld.lld` links them into a ROM image from a linker
   script. Using lwasm/lwlink is fine along the way but not at the end.
3. **Correct code.** Every applicable GCC `c-torture/execute` test passes
   in an emulator for each CPU. Applicable means it fits in 64 KB and
   needs no hosted libc. Each failure is fixed or listed with a reason.
4. **Reasonable code.** On a shared benchmark set, code size at `-Oz` is no
   worse than CMOC or gcc6809 for the 6809.
5. **HD6309 pays off.** `-mcpu=6309` uses the extra registers and
   instructions and gives measurably smaller or faster code than
   `-mcpu=6809` on the same benchmarks. 6809 code keeps running unchanged
   on an HD6309 in emulation mode.
6. **Rust.** A `no_std` crate using `core` builds for `m6809` and `m6800`
   and runs in an emulator.
7. **Solartron proof.** At least one non-trivial routine on each processor,
   rewritten in C or Rust and built with this toolchain, passes the
   matching oracle tests in `../solartron` and runs in place of the
   original code in `solartron-emu`.
8. **Fit for upstream.** The code follows LLVM coding standards, has lit
   tests for every lowering, builds warning-free, and documents its ABI,
   which makes it reasonable to post an RFC for experimental targets.

## Not goals (for now)

- Hosted libc, an OS, or a self-hosting compiler
- Hardware floating point (there isn't any; soft-float only)
- Memory beyond 64 KB through banking or an MMU
- Matching the CMOC or gcc6809 calling convention, unless it becomes
  necessary for linking existing code
- 6800/6802 support beyond what the HD6301 backend gets for free

## Milestones

| # | Milestone | Proves |
|---|-----------|--------|
| M0 | `llc` emits 6809 assembly for straight-line code | Backend skeleton works (**done**) |
| M1 | Branches, calls, globals, shifts, mul/div libcalls, i32 | Real C functions compile (**done**) |
| M2 | Clang target, lwasm-compatible output, crt0, runtime, emulator test loop | C runs end to end (**done**) |
| M3 | c-torture passing for 6809 | Correctness (**done**: every applicable test passes at -O0, -O2 and -Oz; the rest are skipped or listed in tools/torture/known-failures.txt) |
| M4 | MC encoder, ELF relocations, lld | No outside tools needed (**done**) |
| M5 | HD6309 subtarget | Extended CPU support |
| M6 | M6800 backend for the HD6301 | Second backend |
| M7 | Rust `core` on both backends | Rust |
| M8 | Solartron routines replaced on both CPUs | Real-world proof |
| M9 | Clean-up and RFC | Upstream |
