; int setjmp(jmp_buf env)            env in D
; void longjmp(jmp_buf env, int val) env in D, val on the stack
;
; jmp_buf: 0 SP as it is after setjmp returns, 2 the return address. There
; are no callee-saved registers.

	.text
	.globl	setjmp
	.type	setjmp,@function
setjmp:
	xgdx			; X = env
	pula
	pulb			; D = the return address; SP as after the return
	sts	0,x
	std	2,x
	pshb
	psha
	clra
	clrb
	rts
	.size	setjmp, .-setjmp

	.globl	longjmp
	.type	longjmp,@function
longjmp:
	tsx
	ldx	2,x		; val
	xgdx			; X = env, D = val
	subd	#0
	bne	1f
	incb			; longjmp(env, 0) makes setjmp return 1
1:	lds	0,x
	ldx	2,x
	jmp	0,x
	.size	longjmp, .-longjmp
