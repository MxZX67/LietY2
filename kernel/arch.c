#include "kernel.h"

struct __attribute__((packed)) gdt_ptr { uint16_t limit; uint64_t base; };
struct __attribute__((packed)) tss64 {
    uint32_t reserved0;
    uint64_t rsp0, rsp1, rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap;
};
struct __attribute__((packed)) idt_entry {
    uint16_t off_lo;
    uint16_t sel;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t off_mid;
    uint32_t off_hi;
    uint32_t zero;
};

extern void arch_load_gdt(const struct gdt_ptr *p);
extern void arch_load_idt(const void *p);
extern void syscall_int80(void);

static uint64_t gdt[7];
static struct tss64 tss __attribute__((aligned(16)));
static uint8_t tss_stack[8192] __attribute__((aligned(16)));
static struct idt_entry idt[256] __attribute__((aligned(16)));
static struct { uint16_t limit; uint64_t base; } idtr;

static void set_tss_desc(uint64_t base, uint32_t limit){
    uint64_t lo = (uint64_t)(limit & 0xFFFF)
                | ((uint64_t)(base & 0xFFFFFF) << 16)
                | (9ULL << 40)
                | (1ULL << 47);
    lo |= (uint64_t)((limit >> 16) & 0xF) << 48;
    lo |= (uint64_t)((base >> 24) & 0xFF) << 56;
    gdt[5] = lo;
    gdt[6] = base >> 32;
}

static void set_gate(int n, void *fn, uint8_t attr){
    uint64_t a=(uint64_t)fn;
    idt[n].off_lo=(uint16_t)a;
    idt[n].sel=0x08;
    idt[n].ist=0;
    idt[n].type_attr=attr;
    idt[n].off_mid=(uint16_t)(a>>16);
    idt[n].off_hi=(uint32_t)(a>>32);
    idt[n].zero=0;
}

void arch_init(void){
    gdt[0]=0;
    gdt[1]=0x00AF9A000000FFFFULL;
    gdt[2]=0x00AF92000000FFFFULL;
    gdt[3]=0x00AFF2000000FFFFULL;
    gdt[4]=0x00AFFA000000FFFFULL;

    tss.rsp0=(uint64_t)(tss_stack+sizeof(tss_stack));
    tss.iomap=sizeof(tss);
    set_tss_desc((uint64_t)&tss,(uint32_t)sizeof(tss)-1);

    struct gdt_ptr gp={(uint16_t)(sizeof(gdt)-1),(uint64_t)gdt};
    arch_load_gdt(&gp);
    __asm__ volatile("ltr %0"::"r"((uint16_t)0x28));

    for(int i=0;i<256;i++) set_gate(i,(void*)0,0x8E);
    set_gate(0x80,syscall_int80,0xEE);

    idtr.limit=sizeof(idt)-1;
    idtr.base=(uint64_t)idt;
    arch_load_idt(&idtr);
}
