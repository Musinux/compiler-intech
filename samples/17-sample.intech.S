	.globl	main
main:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	$2, -8(%rbp)
	movq	$3, -16(%rbp)
	movq	$2, -24(%rbp)
	movq	-8(%rbp), %rax
	cmpq	%rax, -16(%rbp)
	jge	.L2
.L3:
	movq	-16(%rbp), %rax
	cmpq	%rax, -24(%rbp)
	jl	.L1
.L2:
	movq	-8(%rbp), %rax
	cmpq	%rax, -24(%rbp)
	jle	.L0
.L1:
	movq	$4, -8(%rbp)
.L0:
	movq	$4, -16(%rbp)
