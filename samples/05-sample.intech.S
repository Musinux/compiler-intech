	.globl	main
operation:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$24, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rax
	addq	-16(%rbp), %rax
	movq	-8(%rbp), %rbx
	subq	-16(%rbp), %rbx
	movq	%rax, %r10
	mulq	%rbx, %r10
	movq	-8(%rbp), %rax
	addq	%r10, %rax
main:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movq	$0, -8(%rbp)
.L0:
	cmpq	$100, -8(%rbp)
	jle	.L2
.L1:
	cmpq	$10, -8(%rbp)
	jl	.L3
.L4:
	movq	-8(%rbp), %rbx
	mulq	$2, %rbx
	movq	$1, %r10
	mulq	$3, %r10
	movq	%rbx, %rax
	addq	%r10, %rax
	movq	%rax, -8(%rbp)
.L3:
	movq	-8(%rbp), %rbx
	subq	$1, %rbx
	movq	%rbx, -8(%rbp)
	jmp	.L0
.L2:
	movq	-8(%rbp), %r10
	addq	$1, %r10
	movq	-8(%rbp), %rdi
	movq	%r10, %rsi
	call	operation
