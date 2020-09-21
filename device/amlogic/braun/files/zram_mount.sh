#!/system/bin/sh
echo $((500*1024*1024))  > /sys/block/zram0/disksize
mkswap /dev/block/zram0
swapon /dev/block/zram0
sleep 1
mkfs.ext2 -b 4096 /dev/block/zram0
sleep 1
mount -t rootfs -o remount -rw rootfs /
mkdir /swap_zram0
sleep 1
mount -t rootfs -o remount -r rootfs /
mount -t ext2 -o rw /dev/block/zram0 /swap_zram0
