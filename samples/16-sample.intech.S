	.globl	main
main:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$24, %rsp
	movq	$0, -8(%rbp)
	movq	$0, -16(%rbp)
.L0:
	movq	$1, %rax
	cmpq	$1, %rax
	jne	.L2
.L1:
	movq	-8(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -8(%rbp)
	cmpq	$1000, -8(%rbp)
	jge	.L3
.L4:
.L5:
	cmpq	$10000, -16(%rbp)
	jl	.L7
.L6:
	movq	-8(%rbp), %rax
	subq	$1, %rax
	movq	%rax, -8(%rbp)
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rax
	cmpq	%rax, -8(%rbp)
	jne	.L8
.L9:
	cmpq	$13, -16(%rbp)
	jne	.L10
.L11:
	cmpq	$13, -8(%rbp)
	jne	.L14
.L13:
	movq	$1000, %rax
	leave
	ret
	jmp	.L12
.L14:
	movq	$100, %rax
	leave
	ret
.L12:
.L10:
.L8:
	jmp	.L5
.L7:
	movq	$20, %rax
	leave
	ret
.L3:
	jmp	.L0
.L2:
