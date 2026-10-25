/* Memory map for m6809-sim: flat RAM from $0100 to $FEFF, the stack at the
   top, the simulator's ports at $FF00-$FF02. */
ENTRY(_start)

MEMORY {
  RAM (rwx) : ORIGIN = 0x0100, LENGTH = 0xFE00
}

SECTIONS {
  .text : {
    KEEP(*(.text.crt0))
    *(.text .text.*)
  } > RAM
  .rodata : { *(.rodata .rodata.*) } > RAM
  .init_array : {
    __init_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) .init_array))
    __init_array_end = .;
  } > RAM
  .fini_array : {
    __fini_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) .fini_array))
    __fini_array_end = .;
  } > RAM
  .data : {
    __data_start = .;
    *(.data .data.*)
    __data_end = .;
  } > RAM
  __data_load_start = LOADADDR(.data);
  .bss (NOLOAD) : {
    __bss_start = .;
    *(.bss .bss.* COMMON)
    __bss_end = .;
  } > RAM
  __heap_start = .;
  __stack_top = 0xFF00;
}
