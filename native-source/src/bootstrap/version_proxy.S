/* Windows x64: shadow space + four integer and four FP argument registers.
   Tail jump restores the incoming stack, including all stack arguments and
   the caller's return address. SEH describes the resolver's stack frame. */
.text
.macro VERSION_THUNK index, failure=0
.globl gf2vr_version_\index
.def gf2vr_version_\index; .scl 2; .type 32; .endef
.seh_proc gf2vr_version_\index
gf2vr_version_\index:
    movq gf2vr_version_targets+8*\index(%rip), %rax
    testq %rax,%rax
    jnz 2f
    subq $168,%rsp
    .seh_stackalloc 168
    .seh_endprologue
    movq %rcx,32(%rsp)
    movq %rdx,40(%rsp)
    movq %r8,48(%rsp)
    movq %r9,56(%rsp)
    movdqu %xmm0,64(%rsp)
    movdqu %xmm1,80(%rsp)
    movdqu %xmm2,96(%rsp)
    movdqu %xmm3,112(%rsp)
    movl $\index,%ecx
    call gf2vr_version_resolve
    movq 32(%rsp),%rcx
    movq 40(%rsp),%rdx
    movq 48(%rsp),%r8
    movq 56(%rsp),%r9
    movdqu 64(%rsp),%xmm0
    movdqu 80(%rsp),%xmm1
    movdqu 96(%rsp),%xmm2
    movdqu 112(%rsp),%xmm3
    testq %rax,%rax
    jnz 1f
    movl $\failure,%eax
    addq $168,%rsp
    ret
1:  addq $168,%rsp
2:  jmp *%rax
.seh_endproc
.endm
VERSION_THUNK 0
VERSION_THUNK 1
VERSION_THUNK 2
VERSION_THUNK 3
VERSION_THUNK 4
VERSION_THUNK 5
VERSION_THUNK 6
VERSION_THUNK 7
VERSION_THUNK 8
/* Missing resolver must not falsely report success from flag-returning APIs. */
VERSION_THUNK 9,4
VERSION_THUNK 10,4
VERSION_THUNK 11,65536
VERSION_THUNK 12,65536
VERSION_THUNK 13
VERSION_THUNK 14
VERSION_THUNK 15
VERSION_THUNK 16
