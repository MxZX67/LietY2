CROSS ?=
CC := $(CROSS)gcc
LD := $(CROSS)ld
CFLAGS := -m64 -ffreestanding -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float -O2 -Wall -Wextra -I.
LDFLAGS := -T linker.ld -nostdlib
OBJS := build/entry.o build/main.o build/arch.o build/disk.o build/fs.o build/proc.o build/user.o

all: LietY2-x86_64.iso

build:
	mkdir -p build/isodir/boot/grub

build/entry.o: kernel/entry.S | build
	$(CC) -m64 -c $< -o $@

build/main.o: kernel/main.c kernel/kernel.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/arch.o: kernel/arch.c kernel/kernel.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/disk.o: kernel/disk.c kernel/kernel.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/fs.o: kernel/fs.c kernel/kernel.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/proc.o: kernel/proc.c kernel/kernel.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/user.o: user/init.S | build
	$(CC) -m64 -ffreestanding -mno-red-zone -c $< -o $@

build/kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

LietY2-x86_64.iso: build/kernel.elf boot/grub/grub.cfg
	mkdir -p build/isodir/boot/grub
	cp build/kernel.elf build/isodir/boot/LietY2.elf
	cp boot/grub/grub.cfg build/isodir/boot/grub/grub.cfg
	grub-file --is-x86-multiboot2 build/kernel.elf
	grub-mkrescue -o $@ build/isodir
	truncate -s 20G $@

clean:
	rm -rf build LietY2-x86_64.iso LietY2-disk.img

.PHONY: all clean
