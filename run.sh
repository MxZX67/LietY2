#!/bin/sh
set -eu
make
if [ ! -f LietY2-disk.img ]; then
    truncate -s 64M LietY2-disk.img
fi
qemu-system-x86_64 \
    -M q35 -m 256M \
    -cdrom LietY2-x86_64.iso \
    -drive file=LietY2-disk.img,if=none,id=disk,format=raw \
    -device ich9-ahci,id=sata0 \
    -device ide-hd,bus=sata0.0,drive=disk \
    -boot d -serial stdio
