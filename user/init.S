.section .user.text
.global user_init
.type user_init,@function
user_init:
    lea msg(%rip),%rdi
    mov $1,%rax
    mov $msg_len,%rsi
    int $0x80

    mov $2,%rax
    int $0x80
    lea pidmsg(%rip),%rdi
    mov $1,%rax
    mov $pidmsg_len,%rsi
    int $0x80

    lea fname(%rip),%rdi
    mov $9,%rsi
    lea fdata(%rip),%rdx
    mov $fdata_len,%r10
    mov $5,%rax
    int $0x80

    lea fname(%rip),%rdi
    mov $9,%rsi
    mov $0x802000,%rdx
    mov $256,%r10
    mov $4,%rax
    int $0x80

    mov $0x802000,%rdi
    mov %rax,%rsi
    cmp $256,%rsi
    jbe 1f
    mov $256,%rsi
1:
    mov $1,%rax
    int $0x80

    xor %rdi,%rdi
    mov $3,%rax
    int $0x80
2:
    hlt
    jmp 2b

.section .user.rodata
msg: .ascii "INIT.LYE: jestem w ring3.\n"
.equ msg_len,.-msg
pidmsg: .ascii "INIT: getpid() i filesystem syscall dzialaja.\n"
.equ pidmsg_len,.-pidmsg
fname: .ascii "RING3.TXT"
fdata: .ascii "Ring3 napisal ten plik na prawdziwym dysku.\n"
.equ fdata_len,.-fdata
