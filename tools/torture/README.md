# c-torture on m6809-sim

Runs GCC's `gcc.c-torture/execute` tests through clang, ld.lld and
`m6809-sim`.

## Setup

The tests come from a sparse checkout of GCC in `third_party/gcc`:

```sh
git clone --filter=blob:none --no-checkout https://github.com/gcc-mirror/gcc.git third_party/gcc
cd third_party/gcc
git sparse-checkout set --no-cone '/gcc/testsuite/gcc.c-torture/execute/' \
  '/gcc/testsuite/gcc.dg/tree-ssa/pr105777.c' \
  '/gcc/testsuite/gcc.dg/tree-ssa/pr30314.c' '/gcc/testsuite/gcc.dg/pr98304-1.c'
git checkout cd7a85774a992c4759ebd03a26ce03cbb934e4d9
```

(The three `gcc.dg` files are included by tests.) Build clang, lld and the
simulator, and `make -C runtime install`.

## Running

```sh
tools/torture/run.py -O2            # all tests, results in results-O2-6809.txt
tools/torture/run.py -O0 pr12345    # one test
tools/torture/run.py -O2 --verify   # also run the machine verifier
```

A test is applicable when it fits in 64K and needs nothing a 16-bit target
without an OS cannot have: tests requiring `int32plus`, trampolines and so
on are skipped from their `dg-` directives. `known-failures.txt` lists the
applicable tests that fail for a reason other than a compiler bug, such as
assuming a 32-bit `int` or using a GCC extension clang lacks.
