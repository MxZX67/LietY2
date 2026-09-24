# Architecture

LietY2 is a standalone x86_64 hobby OS. It does not boot Linux, BSD, Windows, or another OS userspace.

The current kernel is intentionally small:
- GRUB loads a Multiboot2 ELF kernel.
- _start establishes a private kernel stack and calls kmain.
- kmain owns the VGA console and PS/2 polling shell.
- COM1 mirrors output for automated QEMU tests.

The 20 GiB ISO is deliberately sparse/logical. The ISO filesystem is small, then the file is extended with zero bytes to exactly 20 GiB. This makes the joke measurable without requiring 20 GiB of real storage.

The separate LietY2-disk.img is the future persistent OS/data disk. The current milestone does not pretend that a RAM dictionary is a disk filesystem; disk drivers are a later milestone.
