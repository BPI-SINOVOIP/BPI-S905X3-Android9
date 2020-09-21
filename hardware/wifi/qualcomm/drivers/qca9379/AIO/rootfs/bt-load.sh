#!/bin/bash

TOPDIR=`pwd`
MODULE_PATH=${TOPDIR}/lib/modules
#MODULE_PATH=./lib/modules

BT_IF_TYPE=$1

echo "===================================="
echo " install backports for bluetooth..."
echo "===================================="

lsmod | grep compat || insmod $MODULE_PATH/compat.ko
lsmod | grep cfg80211 || insmod $MODULE_PATH/cfg80211.ko

## remove bluetooth again
rmmod bluetooth

if [ "${BT_IF_TYPE}" == "USB" ] || [ "${BT_IF_TYPE}" == "usb" ]; then 
    
    insmod $MODULE_PATH/bluetooth.ko  
    insmod $MODULE_PATH/rfcomm.ko
    insmod $MODULE_PATH/bnep.ko   
    insmod $MODULE_PATH/ath3k.ko   
    insmod $MODULE_PATH/btusb.ko   

else

### default as UART interface
    insmod $MODULE_PATH/bluetooth.ko   
    insmod $MODULE_PATH/rfcomm.ko
    insmod $MODULE_PATH/bnep.ko   
    insmod $MODULE_PATH/hci_uart.ko
    insmod $MODULE_PATH/ath3k.ko   
    #insmod $MODULE_PATH/btsdio.ko  
fi

insmod $MODULE_PATH/hidp.ko
modprobe hid_generic

sudo /etc/init.d/bluetooth start 

sleep 1

sudo hciconfig 
