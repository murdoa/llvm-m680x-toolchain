# Targets

| `--target` | CPU | `-mcpu` | Predefined macros |
|---|---|---|---|
| `m6809` | Motorola 6809 | `6809` (default) | `__m6809__`, `__M6809__` |
| `m6809` | Hitachi HD6309 | `6309` | also `__hd6309__`, `__HD6309__` |
| `m6800` | Hitachi HD6301/HD6303 | `6301` (default) | `__m6800__`, `__M6800__`, `__hd6301__`, `__HD6301__` |

The HD6309 runs in 6809 emulation mode: code built with `-mcpu=6309` uses
its extra registers and instructions, and 6809 code runs on it unchanged.

The `m6800` target needs the HD6301's additions (`xgdx`, `abx`, `mul`,
`aim`/`oim`), so its code does not run on a plain 6800 or 6802.

## What you get

- C (C23 and earlier) and C++ (up to C++26, freestanding, no exceptions or
  RTTI) with clang; Rust `no_std` with `core` and `alloc` (see
  [rust.md](rust.md)).
- The integrated assembler and `ld.lld`, producing ELF. `llvm-objcopy -O
  binary` turns that into a ROM image.
- A runtime per target: `crt0`, the compiler-rt builtins (soft float,
  32- and 64-bit arithmetic), a small libc (`string.h`, `ctype.h`,
  `stdlib.h` with `malloc`, `printf`, `setjmp`, `math.h`), and the
  freestanding parts of libc++.

## Types

`char` is 8 bits and signed; `short` and `int` 16; `long` 32; `long long`
64; pointers and `size_t` 16. `float` is IEEE single, and `double` and `long
double` are IEEE double, both in software. Big-endian, and every type is
byte-aligned.

## Calling convention

Both targets share one C calling convention.

- The first argument goes in B (8 bits) or D (16 bits, or the high word of
  a 32-bit one). The rest go on the stack, packed, the first at the lowest
  address just above the return address. Variadic arguments always go on
  the stack.
- Structs and unions are passed by value in the argument area and returned
  through a hidden pointer, passed as the first argument.
- Results: 8 bits in B, 16 in D, 32 (`long`, `float`) with the high word in
  D and the low word in X. Anything wider, such as `long long` and `double`,
  is returned through a hidden pointer.
- 6809: U is callee-saved and is the frame pointer when one is needed; A,
  B, X and Y are caller-saved; DP and CC are never allocated. On the HD6309
  W is caller-saved and MD is never changed.
- HD6301: A, B and X are caller-saved. A function with variable-sized
  locals keeps its frame pointer in the word `__m6800_fp`, which it saves
  and restores.

The full description, including the faster convention used between
internal functions, is in the backends' READMEs in the LLVM fork:
`llvm/lib/Target/M6809/README.md` and `llvm/lib/Target/M6800/README.md`.

## Interrupt handlers

`__attribute__((interrupt))` makes a function an interrupt handler that
returns with `rti`. It takes no parameters and returns `void`. The CPU saves
the registers (all of them on the 6809 for IRQ, NMI and SWI; A, B, X and CC
on the HD6301), so the handler saves none of its own, except W on the
HD6309. The 6809's FIRQ, which saves only CC and PC, is not supported. The
handler is not put in the vector table for you: see
[bare-metal.md](bare-metal.md).

## Direct page (6809)

On the 6809 a variable in a `.dp` or `.dp.*` section is reached with a
one-byte address (`ldb <var`), a byte shorter and a cycle faster than the
full address. Put a variable there with
`__attribute__((section(".dp.bss")))` (zero-initialised) or `".dp.data"`,
and place those sections in the 256-byte page that DP points at (DP is 0
after reset, and the compiler never changes it).

With `-mllvm -m6809-direct-page-bytes=N` the backend chooses the variables
itself, up to N bytes, picking those that save the most bytes. The choice
is per module, so use it with LTO (`-flto`) for a whole program.

The `m6800` target does not use `.dp` sections yet.
