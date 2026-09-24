#pragma once
#include <stdint.h>
#include <stddef.h>

struct syscall_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
};

void puts(const char *s);
void putc(char c);
void clear_screen(void);
void shell_prompt(void);

void arch_init(void);
void enter_user(uint64_t entry, uint64_t user_stack_top);

int disk_init(void);
int disk_read(uint32_t lba, void *buf);
int disk_write(uint32_t lba, const void *buf);
const char *disk_backend(void);

int fs_mount(void);
int fs_format(void);
int fs_info(uint32_t *files, uint32_t *used_blocks, uint32_t *total_blocks);
int fs_list(void);
int fs_read_file(const char *name, void *buf, uint32_t cap, uint32_t *out_len);
int fs_write_file(const char *name, const void *data, uint32_t len);
int fs_delete(const char *name);

void proc_init(void);
int proc_run_init(void);
void proc_list(void);
uint32_t proc_current_pid(void);
void proc_mark_exit(void);

uint64_t syscall_handle(struct syscall_frame *f);
