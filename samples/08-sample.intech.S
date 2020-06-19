	.globl	main
main:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movq	$1, -8(%rbp)
	movq	$2, -16(%rbp)
	movq	$2, -24(%rbp)
	movq	-8(%rbp), %rax
	cmpq	%rax, -16(%rbp)
	jl	.L1
.L2:
	movq	$1, %rax
	addq	$3, %rax
	cmpq	%rax, -16(%rbp)
	jg	.L0
.L3:
	movq	-24(%rbp), %rax
	mulq	$3, %rax
	movq	%rax, %rbx
	addq	$1, %rbx
	movq	$3, %rax
	mulq	$4, %rax
	movq	%rax, %r10
	addq	$-3, %r10
	cmpq	%rbx, %r10
	je	.L0
.L1:
	movq	-8(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -8(%rbp)
.L0:
	movq	$3, %rbx
	mulq	$4, %rbx
	movq	%rbx, %r10
	addq	$-3, %r10
	cmpq	$1, %r10
	je	.L4
.L5:
	movq	-8(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -8(%rbp)
.L4:
