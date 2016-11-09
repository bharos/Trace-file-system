make
umount /mnt/trfs
rmmod trfs
insmod trfs.ko
lsmod
mount -t trfs -o tfile=/tmp/tfile1.txt ../../trmount /mnt/trfs
