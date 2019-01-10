echo "7 6 1 7" > /proc/sys/kernel/printk
insmod dev_driver.ko
mknod /dev/dev_driver c 242 0
chmod 777 /dev/dev_driver
#./app
