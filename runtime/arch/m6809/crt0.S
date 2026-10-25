; crt0 for the 6809: set up the stack, clear .bss, copy .data from its load
; address, run main, then exit with its result.
;
; The linker script provides __stack_top, __bss_start/__bss_end and
; __data_start/__data_end/__data_load_start. They are weak, so a program
; linked without them still runs (with S starting at 0 and nothing to copy).

	.section .text.crt0, "ax"
	.globl	_start
	.type	_start, @function
_start:
	lds	#__stack_top
	; Zero .bss.
	ldx	#__bss_start
1:	cmpx	#__bss_end
	bhs	2f
	clr	,x+
	bra	1b
	; Copy .data if it is loaded somewhere else (from ROM).
2:	ldx	#__data_load_start
	ldy	#__data_start
	cmpx	#__data_start
	beq	4f
3:	cmpy	#__data_end
	bhs	4f
	lda	,x+
	sta	,y+
	bra	3b
4:	jsr	__libc_init_array
	ldd	#0		; argc
	pshs	d		; argv = NULL
	jsr	main
	leas	2,s
	jsr	exit
5:	bra	5b
	.size	_start, .-_start

	.weak	__stack_top
	.weak	__bss_start
	.weak	__bss_end
	.weak	__data_start
	.weak	__data_end
	.weak	__data_load_start
