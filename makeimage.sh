#!/bin/bash
dd bs=1M count=1 seek=200000 if=/dev/zero of=rawdisk.dd
echo -e "n\np\n1\n\n+2000\nn\np\n2\n\n+125000000\nn\np\n3\n\n+125000000\nn\np\n4\n\n+125000000\np\nw\n" | fdisk rawdisk.dd
echo -e "t\n1\n6\nw\n" | fdisk rawdisk.dd
echo -e "t\n2\n40\nw\n" | fdisk rawdisk.dd
echo -e "t\n3\n40\nw\n" | fdisk rawdisk.dd
echo -e "t\n4\n40\nw\n" | fdisk rawdisk.dd
echo -e "a\n1\nw\n" | fdisk rawdisk.dd
echo -e "p\nq\n" | fdisk rawdisk.dd
dd if=./syslinux-3.86/mbr/mbr.bin of=rawdisk.dd conv=notrunc
#exit
#Next steps are seriosly dangerous, better do those by hand. you have been warned
losetup -f ./rawdisk.dd
partprobe /dev/loop0
mkfs.msdos /dev/loop0p1
./syslinux-3.86/linux/syslinux /dev/loop0p1
rm -rf /tmp/msramdump_disk
mkdir /tmp/msramdump_disk
mount /dev/loop0p1 /tmp/msramdump_disk
cp msramdmp.c32 syslinux.cfg /tmp/msramdump_disk/
sleep 2
sync
sleep 2
umount /dev/loop0p1 
losetup -d /dev/loop0
