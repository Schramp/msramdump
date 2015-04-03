#!/bin/bash
dd bs=1M count=1 seek=200000 if=/dev/zero of=rawdisk.dd
#create 4 partitions
echo -e "n\np\n1\n\n+2000\nn\np\n2\n\n+125000000\nn\np\n3\n\n+125000000\nn\np\n4\n\n+125000000\np\nw\n" | fdisk rawdisk.dd
#set all partition types correct
echo -e "t\n1\n6\nw\n" | fdisk rawdisk.dd
echo -e "t\n2\n40\nw\n" | fdisk rawdisk.dd
echo -e "t\n3\n40\nw\n" | fdisk rawdisk.dd
echo -e "t\n4\n40\nw\n" | fdisk rawdisk.dd
#print it for checking
echo -e "p\nq\n" | fdisk rawdisk.dd
#Add the syslinux MBR
dd if=./syslinux-3.86/mbr/mbr.bin of=rawdisk.dd conv=notrunc
echo INCOMPLETE, READ THIS SCRIPT TO FIND OUT WHY
exit
#Next steps are seriosly dangerous, better do those by hand. you have been warned
#loopmount the device
losetup -f ./rawdisk.dd
#detect the partitions (might need a kernel setting)
partprobe /dev/loop0
#format the first partition
mkfs.msdos /dev/loop0p1
#install syslinux in the partition
./syslinux-3.86/linux/syslinux /dev/loop0p1
#create a mount-point
rm -rf /tmp/msramdump_disk
mkdir /tmp/msramdump_disk
#mount the partition
mount /dev/loop0p1 /tmp/msramdump_disk
#copy the files
cp msramdmp.c32 syslinux.cfg /tmp/msramdump_disk/
#give the OS time to flush
sleep 2
sync
sleep 2
#unmount the disk
umount /dev/loop0p1 
#remove the loop
losetup -d /dev/loop0
#without the sync and sleep the bootable flag gets lost. More elegant solution
#needed!
sync
sleep 2
#set the bootable flag on the first partition
echo -e "a\n1\nw\n" | fdisk rawdisk.dd
