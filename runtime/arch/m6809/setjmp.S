; int setjmp(jmp_buf env)            env in D
; void longjmp(jmp_buf env, int val) env in D, val at 2,s
;
; jmp_buf: 0 U, 2 S as it is after setjmp returns, 4 the return address.

	.text
	.globl	setjmp
	.type	setjmp,@function
setjmp:
	tfr	d,x
	stu	,x
	leay	2,s
	sty	2,x
	ldy	,s
	sty	4,x
	clra
	clrb
	rts
	.size	setjmp, .-setjmp

	.globl	longjmp
	.type	longjmp,@function
longjmp:
	tfr	d,x
	ldd	2,s
	bne	.Lnonzero
	incb			; longjmp(env, 0) makes setjmp return 1
.Lnonzero:
	ldu	,x
	lds	2,x
	jmp	[4,x]
	.size	longjmp, .-longjmp
