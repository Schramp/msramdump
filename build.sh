#!/bin/bash
if [ ! -e syslinux-3.86 ];   then
	wget https://www.kernel.org/pub/linux/utils/boot/syslinux/3.xx/syslinux-3.86.tar.gz
	tar -xzf syslinux-3.86.tar.gz
	pushd syslinux-3.86
	make
	popd
fi
gcc -m32 -ffreestanding -fno-stack-protector -W -Wall -march=i386 -Os -fomit-frame-pointer -I./syslinux-3.86/com32/include/ -c -o msramdmp.o msramdmp.c
ld -m elf_i386 -Ttext 0x101000 -e _start -o msramdmp.elf ./syslinux-3.86/sample/c32entry.o msramdmp.o ./syslinux-3.86/sample/liboldcom32.a
objcopy -O binary msramdmp.elf msramdmp.c32
