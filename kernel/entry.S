.section .multiboot
.align 8
.long 0xE85250D6
.long 0
.long 24
.long -(0xE85250D6 + 24)
.word 0
.word 0
.long 8

.section .text
.global _start
.global arch_load_gdt
.global arch_load_idt
.global enter_user
.global syscall_int80
.global user_resume_rsp
.type _start,@function
_start:
    cli
    mov $stack_top, %rsp
    and $-16, %rsp
    call kmain
1:
    hlt
    jmp 1b

.type setup_paging,@function
setup_paging:
    lea pml4(%rip), %rdi
    lea pdpt(%rip), %rsi
    lea pd(%rip), %rdx
    movq $0,(%rdi)
    movq $0,(%rsi)
    movq $0,(%rdx)
    movq %rsi,%rax
    orq $0x7,%rax
    movq %rax,(%rdi)
    movq %rdx,%rax
    orq $0x3,%rax
    movq %rax,(%rsi)
    xor %rcx,%rcx
2:
    mov %rcx,%rax
    shl $21,%rax
    orq $0x87,%rax
    mov %rax,(%rdx,%rcx,8)
    inc %rcx
    cmp $512,%rcx
    jb 2b
    mov %rdi,%rax
    mov %rax,%cr3
    ret

.type arch_load_gdt,@function
arch_load_gdt:
    lgdt (%rdi)
    pushq $0x08
    lea 1f(%rip),%rax
    pushq %rax
    lretq
1:
    mov $0x10,%ax
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%ss
    xor %eax,%eax
    mov %ax,%fs
    mov %ax,%gs
    ret

.type arch_load_idt,@function
arch_load_idt:
    lidt (%rdi)
    ret

.type enter_user,@function
enter_user:
    mov %rsp,user_resume_rsp(%rip)
    mov $0x18,%ax
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%fs
    mov %ax,%gs
    pushq $0x18
    pushq %rsi
    pushfq
    popq %rax
    and $~0x3200,%rax
    or $0x002,%rax
    pushq %rax
    pushq $0x20
    pushq %rdi
    iretq

.type syscall_int80,@function
syscall_int80:
    cld
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    mov %rsp,%rdi
    call syscall_handle
    test %rax,%rax
    jnz 3f
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
    iretq
3:
    mov user_resume_rsp(%rip),%rsp
    ret

.section .bss
.align 4096
pml4: .skip 4096
pdpt: .skip 4096
pd: .skip 4096
.align 16
stack: .skip 32768
stack_top:
.global user_resume_rsp
user_resume_rsp: .quad 0

.section .note.GNU-stack,"",@progbits
