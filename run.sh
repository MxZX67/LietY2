#!/bin/sh
set -eu
make
if [ ! -f LietY2-disk.img ]; then truncate -s 64M LietY2-disk.img; fi
qemu-system-x86_64 -m 256M -cdrom LietY2-x86_64.iso -drive file=LietY2-disk.img,format=raw,if=ide -serial stdio
