CROSS ?=
CC := $(CROSS)gcc
LD := $(CROSS)ld
CFLAGS := -m64 -ffreestanding -fno-pic -fno-stack-protector -mno-red-zone -O2 -Wall -Wextra
LDFLAGS := -T linker.ld -nostdlib

all: LietY2-x86_64.iso

build/kernel.o: kernel/main.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/entry.o: kernel/entry.S
	mkdir -p build
	$(CC) -m64 -c $< -o $@

build/kernel.elf: build/entry.o build/kernel.o linker.ld
	$(LD) $(LDFLAGS) -o $@ build/entry.o build/kernel.o

LietY2-x86_64.iso: build/kernel.elf boot/grub/grub.cfg
	mkdir -p build/isodir/boot/grub
	cp build/kernel.elf build/isodir/boot/LietY2.elf
	cp boot/grub/grub.cfg build/isodir/boot/grub/grub.cfg
	grub-file --is-x86-multiboot2 build/kernel.elf
	grub-mkrescue -O i386-pc -o $@ build/isodir
	truncate -s 20G $@

clean:
	rm -rf build LietY2-x86_64.iso LietY2-disk.img
