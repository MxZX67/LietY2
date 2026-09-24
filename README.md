# LietY2

Standalone x86_64 hobby operating system: DOS-like shell, real disk I/O, LYFS filesystem, ring-3 userspace, and syscall ABI.

## LietY2 2.0

The current 2.0 milestone contains:

- Multiboot2 + GRUB boot
- x86_64 long-mode kernel with its own identity page tables for the first 1 GiB
- VGA text console and PS/2 keyboard polling
- COM1 serial output for QEMU/CI
- PCI mechanism #1 probing
- AHCI SATA driver with one command slot/port
- ATA PIO 28-bit fallback
- LYFS v1 on the persistent secondary disk
- 64 fixed inodes, 4 KiB blocks, bitmap allocation, file create/read/delete
- DOS-like commands: DIR, TYPE, MKFILE, DEL, COPY, REN, TOUCH, FSINFO, CHKDSK, FORMAT C: YES
- process table with PID/state and a real ring-3 INIT
- INT 80h syscall gate with write/getpid/exit/filesystem operations
- CMOS DATE/TIME, CPU identification, PC speaker BEEP, COLOR, CALC
- intentionally pointless commands including FART, DUPA, KURWA, CHUJ, DOOM, MOON, LASER, NUKE and others

The rule remains: technically real, operationally ridiculous.

## Build

Required host tools include GCC/binutils, GRUB tools, xorriso, mtools and QEMU.

Build:

    make

Run:

    ./run.sh

The secondary test disk is LietY2-disk.img and is persistent between runs.

The ISO target is deliberately extended with truncate -s 20G after the real bootable image is produced. Its logical size is exactly 20 GiB while the extra zero area is sparse and does not require 20 GiB of allocated storage.

## Current limitations

This is not yet a production OS. The scheduler is cooperative/single-task at this stage, virtual memory is intentionally broad for the ring-3 milestone, LYFS is flat, and AHCI is polling-based with one port/slot. Per-process page tables, interrupts/timer scheduling, a proper executable loader from LYFS, USB, networking, installer, framebuffer GUI and desktop remain future work.
