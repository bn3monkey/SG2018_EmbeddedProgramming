echo "7 6 1 7" > /proc/sys/kernel/printk
insmod dev_driver.ko
mknod /dev/dev_driver c 242 0

./app 1 25 1000
./app 5 25 2000
./app 10 25 3000
./app 50 25 4000
./app 100 25 5000
./app 1 50 6000
./app 5 50 7000
./app 10 50 8000
./app 50 50 0100
./app 100 50 0200
./app 1 75 0300
./app 5 75 0400
./app 10 75 0500
./app 50 75 0600
./app 100 75 0700
./app 1 100 0800
./app 5 100 0010
./app 10 100 0020
./app 50 100 0030
./app 100 100 0040
./app 1 25 0050
./app 5 25 0060
./app 10 25 0070
./app 50 25 0080
./app 100 25 0001
./app 1 50 0002
./app 5 50 0003
./app 10 50 0004
./app 50 50 0005
./app 100 50 0006
./app 1 75 0007
./app 5 75 0008
./app 10 75 1000
./app 50 75 2000
./app 100 75 3000