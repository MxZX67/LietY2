ENTRY(_start)
SECTIONS
{
  . = 1M;
  .multiboot : ALIGN(8) { KEEP(*(.multiboot)) }
  .text : ALIGN(16) { *(.text*) }
  .rodata : ALIGN(16) { *(.rodata*) }
  .data : ALIGN(16) { *(.data*) }
  .user 0x400000 : ALIGN(16) {
    __user_start = .;
    KEEP(*(.user.text*))
    KEEP(*(.user.rodata*))
    __user_end = .;
  }
  .bss : ALIGN(4096) { *(COMMON) *(.bss*) }
}
