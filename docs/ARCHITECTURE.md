# LietY2 2.0 architecture

## Boot

GRUB loads a Multiboot2 x86_64 ELF kernel. _start establishes a private kernel stack and installs a minimal identity page map for the first 1 GiB using 2 MiB pages. The page entries are user-accessible for the current ring-3 milestone.

## Console

The kernel writes VGA text memory at 0xB8000, mirrors output to COM1, and polls the legacy PS/2 keyboard controller. Interrupt-driven keyboard input is intentionally deferred.

## Disk stack

The kernel first scans PCI configuration space for an AHCI controller (class 01h/subclass 06h/prog-if 01h). BAR5 is used as the HBA MMIO base. One SATA port with the normal ATA signature is configured with one command slot, one received-FIS area, one command table and one PRDT entry; completion is polled.

When AHCI is unavailable, the disk layer probes the primary ATA channel and uses 28-bit PIO reads/writes.

QEMU attaches LietY2-disk.img to the ICH9 AHCI controller, making filesystem tests exercise the disk path instead of a memory-only fake.

## LYFS v1

The test disk is 64 MiB / 131072 sectors.

| Region | Sectors |
|---|---:|
| reserved | 0 |
| superblock | 1 |
| inode table | 2-17 |
| bitmap | 18-21 |
| data | 22+ |

There are 64 fixed 128-byte inodes. Data blocks are 4 KiB (8 sectors). The filesystem is flat and uses a bitmap plus contiguous block runs. Files can be created, read, overwritten and deleted.

The formatter creates WELCOME.TXT and README.TXT on the disk.

## Processes and ring 3

The kernel maintains a small fixed process table. PID 0 is the kernel. RUNPID starts the linked INIT.LYE image at virtual address 0x400000 with a user stack near 0x804000.

The TSS provides RSP0 for privilege transitions. User code executes with CS=0x20 and DS/SS=0x18. The current memory map intentionally does not isolate kernel memory; a future VM subsystem will provide per-process page tables and stricter user/kernel permissions.

## Syscalls

Userspace calls INT 0x80. The IDT gate is DPL3.

| RAX | Operation |
|---:|---|
| 1 | write(ptr, len) |
| 2 | getpid() |
| 3 | exit(code) |
| 4 | fs_read(name, name_len, buf, cap) |
| 5 | fs_write(name, name_len, buf, len) |

Return values are placed in saved RAX. A simple userspace range check currently accepts 0x400000..0x900000.

INIT.LYE exercises these paths, including writing and reading RING3.TXT.

## Intentional absurdity

Absurd commands live in the shell rather than pretending to be kernel services. This keeps the joke layer separate from disk/process mechanisms.

## Next technical milestones

Timer/PIC/APIC and IDT exception handling, physical-page allocator, heap, preemptive scheduler, per-process page tables, executable loader from LYFS, USB HID, network device, installer and framebuffer/GUI.
