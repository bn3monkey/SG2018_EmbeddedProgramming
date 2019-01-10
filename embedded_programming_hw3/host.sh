#sudo -s
#source /root/.bashrc
cd app
make
cd ..
cd module
make
cd ..
chmod 740 device.sh
chmod 740 clean.sh 
adb push module/stopwatch.ko /data/local/tmp
adb push app/app /data/local/tmp
adb push device.sh /data/local/tmp
adb push clean.sh /data/local/tmp
