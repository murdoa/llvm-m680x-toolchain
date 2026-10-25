; crt0 for the 6800: set up the stack, clear .bss, copy .data from its load
; address, run main, then exit with its result.
;
; The linker script provides __stack_top (the byte after the stack),
; __bss_start/__bss_end and __data_start/__data_end/__data_load_start. They
; are weak, so a program linked without them still runs.

	.section .text.crt0, "ax"
	.globl	_start
	.type	_start, @function
_start:
	lds	#__stack_top-1	; SP points at the next free byte
	; Zero .bss.
	ldx	#__bss_start
1:	cpx	#__bss_end
	bhs	2f
	clr	0,x
	inx
	bra	1b
	; Copy .data if it is loaded somewhere else (from ROM).
2:	ldx	#__data_load_start
	cpx	#__data_start
	beq	3f
	ldd	#__data_end
	subd	#__data_start
	pshb
	psha			; n
	pshx			; src
	ldd	#__data_start	; dst
	jsr	memcpy
	ins
	ins
	ins
	ins
3:	jsr	__libc_init_array
	ldx	#0
	pshx			; argv = NULL
	ldd	#0		; argc
	jsr	main
	ins
	ins
	jsr	exit
4:	bra	4b
	.size	_start, .-_start

	.weak	__stack_top
	.weak	__bss_start
	.weak	__bss_end
	.weak	__data_start
	.weak	__data_end
	.weak	__data_load_start
