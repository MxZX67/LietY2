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
.extern kmain
_start:
    cli
    mov $stack_top, %rsp
    xor %rbp, %rbp
    call kmain
1:  hlt
    jmp 1b
.section .bss
.align 16
stack:
    .skip 65536
stack_top:
