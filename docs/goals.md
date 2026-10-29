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
   matching oracle tests in the Solartron workbench and runs in place of
   the original code in its emulator.
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
| M5 | HD6309 subtarget | Extended CPU support (**done**: c-torture passes at -O0, -O2 and -Oz with `-mcpu=6309`, and 6809 code passes unchanged on the HD6309 simulator; -Oz size is 0.92 of CMOC against 0.97 for the 6809, and -O2 code takes 0.66 of the 6809 cycles, see tools/size and tools/cycles) |
| M6 | M6800 backend for the HD6301 | Second backend (**done**: c-torture passes at -O0, -O2 and -Oz, the C++ runtime test runs, and llvm/test/CodeGen/M6800 covers the lowerings with -verify-machineinstrs; see tools/torture/known-failures-m6800.txt) |
| M7 | Rust `core` on both backends | Rust (**done**: `m6809-none-elf` and `m6800-none-elf` build `core` and `alloc`, and a no_std test with traits, `Option`, u64 and f32 runs on the 6809, HD6309 and HD6301 at -O0, -O2 and -Oz; see rust/) |
| M8 | Solartron routines replaced on both CPUs | Real-world proof (**done**: the earthy calibration-transfer check (6809) and the floating acquisition coefficient (HD6301) are C, patched into the ROMs, and both images pass all 193 acceptance scenarios in the Solartron workbench) |
| M9 | Clean-up and RFC | Upstream (**done**: the machine verifier is clean on all of c-torture for the three CPUs, functions get their own subtarget, the code is clang-formatted and builds without warnings, and an RFC was drafted; it has not been posted) |

The full replacement firmware for both boards, built with this toolchain,
lives in the Solartron workbench (`firmware/`), whose flake takes the
toolchain from this repository.
