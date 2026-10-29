# Bare-metal programs

`--config=<target>-sim.cfg` links a program for the simulators. For a real
board you give your own linker script for its memory map, and supply the
few functions the runtime expects from a board.

## What the runtime needs

`crt0` (the `.text.crt0` section, entry point `_start`) sets S to
`__stack_top`, clears `.bss`, copies `.data` from its load address, runs
the constructors, calls `main` and passes its result to `exit`. It reads
these symbols from the linker script:

| Symbol | Meaning |
|---|---|
| `__stack_top` | initial stack pointer (the stack grows down from it) |
| `__bss_start`, `__bss_end` | the range cleared at startup |
| `__data_start`, `__data_end` | where `.data` runs |
| `__data_load_start` | where `.data` is stored (in ROM) |
| `__init_array_start`, `__init_array_end` | C++ constructors, if any |
| `__heap_start` | the start of the `malloc` heap, if you use it |

The board provides:

- `void _exit(int status)`: what to do when `main` returns (loop, reset,
  report to a debugger).
- `int __board_putc(int c)` and `int __board_getc(void)`, if the program
  uses `stdio.h`.

The simulators' versions are in `runtime/sim/sim.c`.

`crt0` does not clear or copy the 6809's direct-page sections. Clear `.dp.bss`
and copy `.dp.data` in your own startup code, or keep them inside the
ranges above.

## An example for a 6809 board

8 KB of RAM at `$0000`, of which the first page is the direct page, and 8 KB
of ROM at `$E000` with the vectors at the top:

```ld
ENTRY(_start)

MEMORY {
  DP  (rw) : ORIGIN = 0x0000, LENGTH = 0x0100
  RAM (rw) : ORIGIN = 0x0100, LENGTH = 0x1F00
  ROM (rx) : ORIGIN = 0xE000, LENGTH = 0x1FF0
  VEC (r)  : ORIGIN = 0xFFF0, LENGTH = 0x0010
}

SECTIONS {
  .dp (NOLOAD) : { *(.dp.bss .dp.bss.*) } > DP
  .text : { KEEP(*(.text.crt0)) *(.text .text.*) } > ROM
  .rodata : { *(.rodata .rodata.*) } > ROM
  .init_array : {
    __init_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) .init_array))
    __init_array_end = .;
  } > ROM
  .data : { __data_start = .; *(.data .data.*) __data_end = .; } > RAM AT > ROM
  __data_load_start = LOADADDR(.data);
  .bss (NOLOAD) : { __bss_start = .; *(.bss .bss.* COMMON) __bss_end = .; } > RAM
  __heap_start = .;
  __stack_top = ORIGIN(RAM) + LENGTH(RAM);

  /* $FFF0: reserved, SWI3, SWI2, FIRQ, IRQ, SWI, NMI, RESET. */
  .vectors : {
    SHORT(0) SHORT(0) SHORT(0) SHORT(0)
    SHORT(irq) SHORT(0) SHORT(0) SHORT(_start)
  } > VEC
}
```

```c
static unsigned char ticks __attribute__((section(".dp.bss")));

__attribute__((interrupt)) void irq(void) { ticks++; }

void _exit(int status) { (void)status; for (;;) {} }

int main(void) { /* ... */ return 0; }
```

```sh
clang --target=m6809 -Oz -T board.ld main.c -o fw.elf
llvm-objcopy -O binary -j .text -j .rodata -j .init_array -j .data -j .vectors fw.elf fw.bin
```

`fw.bin` is the 8 KB ROM image, from `$E000` to `$FFFF`. `.dp` is
`NOLOAD`, so clear it in `main` (or in a startup routine of your own) before
relying on it.

The HD6301 has more vectors (`$FFEA` to `$FFFF`) and on-chip ports and RAM
in the low addresses; see its data sheet for the map.
