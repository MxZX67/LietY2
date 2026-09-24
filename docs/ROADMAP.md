# LietY2 roadmap

## 2.0 baseline

- [x] Multiboot2/GRUB boot
- [x] x86_64 kernel stack
- [x] VGA + COM1 console
- [x] PS/2 polling keyboard
- [x] PCI config-space scan
- [x] AHCI SATA disk I/O
- [x] ATA PIO fallback
- [x] persistent LYFS v1
- [x] DOS-like filesystem commands
- [x] process table + PID
- [x] TSS and ring 3
- [x] INT 80h syscall ABI
- [x] linked INIT userspace program
- [x] QEMU CI smoke test with persistent disk
- [x] logical 20 GiB ISO

## 2.1 kernel work

- [ ] IDT exception stubs with readable panic diagnostics
- [ ] PIT/APIC timer
- [ ] preemptive scheduler and context switching
- [ ] physical page allocator
- [ ] kernel heap
- [ ] per-process page tables and supervisor/user separation
- [ ] safer user-pointer validation
- [ ] real executable loader from LYFS
- [ ] parent/child process state and wait

## 2.2 hardware

- [ ] multiple AHCI ports and command slots
- [ ] 48-bit LBA and larger disks
- [ ] AHCI interrupts and error recovery
- [ ] USB controller + HID
- [ ] framebuffer discovery
- [ ] network device + ARP/IP/ICMP

## 3.x user experience

- [ ] richer shell parser and command redirection
- [ ] pipes
- [ ] environment variables
- [ ] real COPY, REN, wildcard matching and batch files
- [ ] installer and disk boot
- [ ] framebuffer GUI
- [ ] window manager and desktop
- [ ] .LYE applications loaded from the filesystem

## The pointless department

- [x] FART
- [x] DUPA
- [x] KURWA
- [x] CHUJ
- [x] DOOM
- [x] MOON
- [x] LASER
- [x] NUKE
- [x] WHY
- [x] NOTHING
- [x] MATRIX
- [x] RICKROLL
- [ ] boot-time lottery
- [ ] fake antivirus
- [ ] useless screensaver
- [ ] calculator with emotional damage
- [ ] WHY /AGAIN
- [ ] command that changes nothing but takes 30 seconds
