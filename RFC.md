# [RFC] Experimental M6809 and M6800 backends

## Summary

We would like to add two experimental targets:

- **M6809**: the Motorola 6809, with the Hitachi HD6309 as a subtarget
  (`-mcpu=6309`).
- **M6800**: the Motorola 6800 family, aimed at the Hitachi HD6301/HD6303
  (`-mcpu=6301`, the default), the microcontroller version of the 6800.

Both are 8-bit CPUs with 16-bit addresses that are still used in
industrial and instrument equipment, in retro computing (Tandy CoCo,
Dragon, Vectrex, Williams arcade boards) and in teaching. Today their C
compilers are CMOC and the unmaintained gcc6809 port. C++ and Rust have
no compiler for them. These backends bring modern C, C++ and Rust (`core`
and `alloc`) to them, with the integrated assembler and `ld.lld`, so no
outside tools are needed.

The code is at https://github.com/murdoa/llvm-project, branch `m6809`.

## What is there

| | M6809 | M6800 |
|---|---|---|
| Backend | about 9,200 lines, 54 files | about 7,600 lines, 50 files |
| lit tests | 67 (CodeGen, MC) plus LoopUnroll | 50 (CodeGen, MC) |
| MC | encoder, ELF relocations, asm parser, disassembler | encoder, ELF relocations, asm parser, branch relaxation; no disassembler yet |
| clang | target, ABI, driver (bare metal) | target, ABI, driver (bare metal) |
| lld | ELF/M6809 | ELF/M6800 |
| compiler-rt | builtins: multiply, divide, shifts in assembly; the rest in C | the same |

Both backends use SelectionDAG, the greedy register allocator, and the
generic pipeline. The ABI is the same for both and is in each target's
README.md. The 6800 backend was forked from the 6809 one and shares its
design. The 6800 has a single index register and no stack-relative
addressing, so it reaches the stack through X (`tsx; ldd 2,x`): a late
pass, `M6800FrameAccess`, rewrites stack slots into that form.

## How it has been tested

- **GCC c-torture execute.** Every applicable test passes in a cycle-level
  simulator (in-tree tools, not part of this RFC), with the machine
  verifier on (`-verify-machineinstrs`). That is about 1,530
  tests per configuration: the 6809 and the HD6309 at `-O0`, `-O2` and
  `-Oz`, 6809 code run on an HD6309, and the HD6301 at `-O0`, `-O2` and
  `-Oz`. Not applicable means needing more than 64 KB, a hosted libc,
  trampolines, or 32-bit `int`. The remaining expected failures are listed
  with reasons.
- **Size.** At `-Oz` on a shared benchmark set, code is 0.92 of CMOC's size
  on the 6809 and 0.87 on the HD6309.
- **C++20 and Rust.** A C++20 state machine (`consteval`, `enum class`,
  templates) gives the same output as the host build at every
  optimization level, with and without LTO, on all three CPUs. A `no_std`
  Rust crate with traits, `Option`, `u64` and `f32` builds and runs on all
  three at `opt-level` 0, 2 and z. The Rust target specs are not part of
  this RFC.
- **Real firmware.** In the firmware of a Solartron 7081 precision
  voltmeter, which has a 6809 board and an HD6301 board, one routine per
  board was rewritten in C, built with this toolchain, and patched into
  the original ROMs. Both pass the instrument's acceptance tests against
  golden traces from the original firmware. Those traces are
  cycle-sensitive, so each C routine had to match the original's cycle
  count on every path.

## Changes outside the targets

Most changes are the usual target registration: triple, ELF machine and
relocations, clang, lld, compiler-rt, and UpdateTestChecks. A few touch
shared code and would be posted for review on their own:

- `[RegAllocGreedy]` canReassign skipped only the evicting range's
  register, not the registers overlapping it, so an i8 range evicted from
  B could be "reassigned" to A while an i16 range took D.
- `[RegAllocFast]` Allocate uses of single-register classes first.
- `[CodeGen]` Let stack coloring share the slots of demoted call results.
- `[CodeGen]` Keep a dead copy's def dead when the spiller folds it into a
  reload.
- `[SelectionDAG]` Check ADDE on the half type in expandMUL_LOHI; don't
  assert on inline asm values wider than their register class.
- `[AtomicExpand]` Pass the memory order as a 16-bit int on 16-bit targets.

The two RegAlloc fixes, the folding fix and the SelectionDAG ones are
target-independent bugs, which these small register files expose.

## Open questions

- **ELF machine numbers.** No `EM_` value is assigned for either CPU. The
  branch uses provisional values, 0x6809 and 0x6800. We would apply for
  official numbers before this lands, and would welcome advice on how.
- **One target or two?** The 6800 family and the 6809 are not compatible
  at the binary level, and the 6800's single index register leads to a
  different design. Separate targets keep each simpler. Sharing more code,
  such as the MC layer's operand handling, is possible.
- **The original 6800 and 6802.** The M6800 backend targets the HD6301,
  which adds instructions (`xgdx`, `abx`, `mul`, `aim`/`oim`) the 6800
  lacks. Supporting the plain 6800 would need those to be optional.
- **Maintenance.** We are prepared to maintain both targets, and to set up
  a buildbot running the lit tests and the c-torture suite in the
  simulators.

## Next steps

If there is interest, we would post the shared-code fixes first, then each
target in the usual series: target skeleton, MC layer, codegen, clang, lld
and compiler-rt.
