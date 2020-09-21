#!/bin/sh
#

rm -f /lib/firmware/athwlan*.bin
rm -f /lib/firmware/qwlan*.bin
rm -f /lib/firmware/athsetup*.bin
rm -f /lib/firmware/qsetup*.bin
rm -f /lib/firmware/utf*.bin
rm -f /lib/firmware/otp*.bin
rm -f /lib/firmware/fakeboar.bin
rm -f /lib/firmware/bdwlan*.bin
rm -f /lib/firmware/utfbd*.bin

rm -rf /lib/firmware/wlan

cp -rf lib/firmware/wlan   /lib/firmware/
cp -rf lib/firmware/WLAN-firmware/* /lib/firmware/
cp -rf lib/firmware/BT-firmware/* /lib/firmware/ar3k/


