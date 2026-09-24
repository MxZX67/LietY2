# LietY2 roadmap

## Implemented baseline
- Real x86_64 kernel
- Multiboot2/GRUB boot
- VGA text console
- PS/2 keyboard polling
- COM1 serial console
- DOS-like command interpreter
- 20 GiB sparse/logical ISO target
- Persistent 64 MiB secondary disk image in QEMU
- CI build + QEMU smoke test

## Next kernel milestones
1. IDT and exception handlers
2. PIT/APIC timer and preemptive scheduler
3. physical page allocator + kernel heap
4. ring 3 + TSS
5. syscall ABI
6. ATA PIO + AHCI
7. persistent LYFS filesystem
8. executable loader for LYE
9. per-process page tables
10. USB HID
11. framebuffer GUI
12. installer and disk boot
13. networking

## Absurdity backlog
- FART, DOOM, MOON, LASER, NUKE
- fake motivational diagnostics
- random error messages
- contradictory command aliases
- calculator that insults bad arithmetic
- boot-time lottery
- WHY command with existential answers
