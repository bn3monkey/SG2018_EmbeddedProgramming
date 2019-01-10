echo "7 6 1 7" > /proc/sys/kernel/printk
insmod stopwatch.ko
mknod /dev/stopwatch c 242 0

./app