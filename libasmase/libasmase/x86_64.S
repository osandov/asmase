#include <asm-generic/mman-common.h>
#include <sys/syscall.h>

#include "asm.h"

/* XXX: should probably get this programatically. */
#define TASK_SIZE ((1 << 47) - 4096)

	.section .rodata

	.global	arch_trap_instruction
arch_trap_instruction:
	int $0x3
arch_trap_instruction_end:

	.balign	8
	.global	arch_trap_instruction_len
arch_trap_instruction_len:
	.quad	arch_trap_instruction_end-arch_trap_instruction

	.global	arch_bootstrap_code
arch_bootstrap_code:
	/* Save the memfd. */
	movq %rdi, %r14
	movq %rsi, %r15
	/* Map the memfd to its final location. */
	movabsq $MEMFD_ADDR, %rdi
	movq $MEMFD_SIZE, %rsi
	movq $(PROT_READ | PROT_WRITE | PROT_EXEC), %rdx
	movq $(MAP_SHARED | MAP_FIXED), %r10
	movq %r14, %r8
	xorq %r9, %r9
	movq $SYS_mmap, %rax
	syscall
	cmpq $0, %rax
	jl exit
	/* Close the memfd. */
	movq %r14, %rdi
	movq $SYS_close, %rax
	syscall
	cmpq $0, %rax
	jl exit
	/* Jump to the same instruction stream at its new location. */
	movabsq $(MEMFD_ADDR + bootstrap_munmap - arch_bootstrap_code), %rax
	jmp *%rax
	/* Unmap everything before and after the memfd. */
bootstrap_munmap:
	xorq %rdi, %rdi
	movabsq $MEMFD_ADDR, %rsi
	movq $SYS_munmap, %rax
	syscall
	cmpq $0, %rax
	jl exit
	movq $(MEMFD_ADDR + MEMFD_SIZE), %rdi
	movabsq $(TASK_SIZE - MEMFD_ADDR - MEMFD_SIZE), %rsi
	movq $SYS_munmap, %rax
	syscall
	cmpq $0, %rax
	jl exit
	/* Move the stack pointer to the end of the memfd. */
	movq $(MEMFD_ADDR + MEMFD_SIZE), %rsp
	/* Set up seccomp if that was requested. */
	testl %r15d, %r15d
	jz bootstrap_registers
	/* prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0); */
	movq $38, %rdi
	movq $1, %rsi
	xorq %rdx, %rdx
	xorq %r10, %r10
	xorq %r8, %r8
	movq $SYS_prctl, %rax
	syscall
	cmpq $0, %rax
	jl exit
	/* struct sock_filter filter = BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRAP); */
	subq $24, %rsp
	movl $0x6, 16(%rsp)
	movl $0x00030000, 20(%rsp)
	/* struct sock_fprog prog = { .len = 1, .filter = &filter }; */
	movw $1, (%rsp)
	leaq 16(%rsp), %rdi
	movq %rdi, 8(%rsp)
	/* seccomp(SECCOMP_SET_MODE_FILTER, 0, &prog); */
	movq $1, %rdi
	xorq %rsi, %rsi
	movq %rsp, %rdx
	movq $SYS_seccomp, %rax
	syscall
	cmpq $0, %rax
	jl exit
	addq $24, %rsp
	/* Reset registers. */
bootstrap_registers:
	xorq %rax, %rax
	xorq %rcx, %rcx
	xorq %rdx, %rdx
	xorq %rbx, %rbx
	xorq %rbp, %rbp
	xorq %rsi, %rsi
	xorq %rdi, %rdi
	xorq %r8, %r8
	xorq %r9, %r9
	xorq %r10, %r10
	xorq %r11, %r11
	xorq %r12, %r12
	xorq %r13, %r13
	xorq %r14, %r14
	xorq %r15, %r15
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movq %rax, %mm0
	movq %rax, %mm1
	movq %rax, %mm2
	movq %rax, %mm3
	movq %rax, %mm4
	movq %rax, %mm5
	movq %rax, %mm6
	movq %rax, %mm7
	movq %rax, %xmm0
	movq %rax, %xmm1
	movq %rax, %xmm2
	movq %rax, %xmm3
	movq %rax, %xmm4
	movq %rax, %xmm5
	movq %rax, %xmm6
	movq %rax, %xmm7
	movq %rax, %xmm8
	movq %rax, %xmm9
	movq %rax, %xmm10
	movq %rax, %xmm11
	movq %rax, %xmm12
	movq %rax, %xmm13
	movq %rax, %xmm14
	movq %rax, %xmm15
	movl $0x1f80, -4(%rsp)
	ldmxcsr -4(%rsp)
	fninit
	pushfq
	andl $0xfffff32a, (%rsp)
	popfq
	/* We're done; trap. */
	int $0x3
exit:
	movq $1, %rdi
	movq $SYS_exit_group, %rax
	syscall

	.balign	8
	.global	arch_bootstrap_code_len
arch_bootstrap_code_len:
	.quad	.-arch_bootstrap_code
