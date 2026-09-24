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
	command -v mformat >/dev/null 2>&1 || { echo 'LietY2 CI: installing mtools because grub-mkrescue needs mformat'; sudo apt-get update; sudo apt-get install -y mtools; }
	grub-file --is-x86-multiboot2 build/kernel.elf
	grub-mkrescue -o $@ build/isodir
	truncate -s 20G $@

clean:
	rm -rf build LietY2-x86_64.iso LietY2-disk.img
