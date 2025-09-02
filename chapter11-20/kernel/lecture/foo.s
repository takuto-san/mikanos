	.text
	.intel_syntax noprefix
	.file	"foo.cpp"
	.globl	_Z3foov                 # -- Begin function _Z3foov
	.p2align	4, 0x90
	.type	_Z3foov,@function
_Z3foov:                                # @_Z3foov
# %bb.0:
	push	rbp
	mov	rbp, rsp
	mov	dword ptr [rbp - 4], 42
	lea	rax, [rbp - 4]
	mov	qword ptr [rbp - 16], rax
	mov	rax, qword ptr [rbp - 16]
	mov	ecx, dword ptr [rax]
	mov	dword ptr [rbp - 20], ecx
	mov	rax, qword ptr [rbp - 16]
	mov	dword ptr [rax], 1
	mov	ecx, dword ptr [rbp - 4]
	mov	dword ptr [rbp - 24], ecx
	mov	rax, qword ptr [rbp - 16]
	mov	qword ptr [rbp - 32], rax
	mov	rax, qword ptr [rbp - 32]
	mov	qword ptr [rbp - 40], rax
	pop	rbp
	ret
.Lfunc_end0:
	.size	_Z3foov, .Lfunc_end0-_Z3foov
                                        # -- End function

	.ident	"clang version 7.0.1-12 (tags/RELEASE_701/final)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym _Z3foov
