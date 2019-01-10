#sudo -s
#source /root/.bashrc
make
cd /
mkdir data/local/tmp
cd ~/embe_1
cp -f HW1_20121592 /data/local/tmp
cp -f build_test_device.sh /data/local/tmp
cp -rf device_driver /data/local/tmp
adb push HW1_20121592 /data/local/tmp
adb push build_test_device.sh /data/local/tmp
adb push device_driver /data/local/tmp
adb push rhythm.txt /data/local/tmp