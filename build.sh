#!/bin/bash
gcc -m32 -ffreestanding -fno-stack-protector -W -Wall -march=i386 -Os -fomit-frame-pointer -I../syslinux-3.61/com32/include/ -c -o msramdmp.o msramdmp.c
ld -m elf_i386 -Ttext 0x101000 -e _start -o msramdmp.elf c32entry.o msramdmp.o liboldcom32.a
objcopy -O binary msramdmp.elf msramdmp.c32
