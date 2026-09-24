.section .multiboot
.align 8
.long 0xE85250D6
.long 0
.long 24
.long -(0xE85250D6 + 24)
.word 0
.word 0
.long 8

.code32
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
    mov $0xE9,%dx
    mov $0x53,%al
    out %al,%dx
    mov $boot_stack_top,%esp
    and $-16,%esp
    call setup_paging32
    lgdt gdt64_ptr
    mov $0xE9,%dx
    mov $0x4C,%al
    out %al,%dx
    ljmp $0x08,$long_mode_start

.type setup_paging32,@function
setup_paging32:
    mov $pml4,%edi
    xor %eax,%eax
    mov $24576/4,%ecx
    rep stosl

    mov $pdpt,%eax
    or $0x7,%eax
    mov %eax,pml4
    mov $pd,%eax
    or $0x7,%eax
    mov %eax,pdpt
    add $0x1000,%eax
    mov %eax,pdpt+8
    add $0x1000,%eax
    mov %eax,pdpt+16
    add $0x1000,%eax
    mov %eax,pdpt+24

    xor %ecx,%ecx
1:
    mov %ecx,%eax
    shl $21,%eax
    or $0x87,%eax
    mov %eax,pd(,%ecx,8)
    movl $0,pd+4(,%ecx,8)
    inc %ecx
    cmp $2048,%ecx
    jb 1b

    mov $pml4,%eax
    mov %eax,%cr3
    mov %cr4,%eax
    or $0x20,%eax
    mov %eax,%cr4

    mov $0xC0000080,%ecx
    rdmsr
    or $0x100,%eax
    wrmsr

    mov %cr0,%eax
    or $0x80000000,%eax
    mov %eax,%cr0
    ret

.code64
.type long_mode_start,@function
long_mode_start:
    mov $0x10,%ax
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%ss
    xor %eax,%eax
    mov %ax,%fs
    mov %ax,%gs
    mov $stack_top,%rsp
    and $-16,%rsp
    mov $0xE9,%dx
    mov $0x4D,%al
    out %al,%dx
    call kmain
    mov $0xE9,%dx
    mov $0x48,%al
    out %al,%dx
1:
    hlt
    jmp 1b

.align 8
gdt64:
    .quad 0
    .quad 0x00AF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt64_end:
gdt64_ptr:
    .word gdt64_end-gdt64-1
    .long gdt64

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
    mov $0x1B,%ax
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%fs
    mov %ax,%gs
    pushq $0x1B
    pushq %rsi
    pushfq
    popq %rax
    and $~0x3200,%rax
    or $0x002,%rax
    pushq %rax
    pushq $0x23
    pushq %rdi
    mov $0xE9,%dx
    mov $0x55,%al
    out %al,%dx
    iretq

.type syscall_int80,@function
syscall_int80:
    mov $0xE9,%dx
    mov $0x4B,%al
    out %al,%dx
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
    sub $8,%rsp
    lea 8(%rsp),%rdi
    call syscall_handle
    mov %rax,%r11
    mov 120(%rsp),%rax
    mov $0xE9,%dx
    mov $0x52,%al
    out %al,%dx
    test %r11,%r11
    jnz 3f
    add $8,%rsp
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
    add $8,%rsp
    iretq
3:
    mov user_resume_rsp(%rip),%rsp
    ret

.section .bss
.align 4096
pml4: .skip 4096
pdpt: .skip 4096
pd: .skip 16384
.align 16
boot_stack: .skip 16384
boot_stack_top:
.align 16
stack: .skip 32768
stack_top:
.global user_resume_rsp
user_resume_rsp: .quad 0

.section .note.GNU-stack,"",@progbits
