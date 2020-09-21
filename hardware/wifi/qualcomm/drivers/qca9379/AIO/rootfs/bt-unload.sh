#!/bin/bash

TOPDIR=`pwd`
MODULE_PATH=${TOPDIR}/lib/modules

BT_IF_TYPE=$1

echo "===================================="
echo " Unloading the BT drivers           "
echo "===================================="

sudo /etc/init.d/bluetooth stop 

sleep 1

rmmod hid_generic
rmmod hidp

if [ "${BT_IF_TYPE}" == "USB" ] || [ "${BT_IF_TYPE}" == "usb" ]; then

    echo "Unload the USB BT ..."
    sudo rmmod bnep
    sudo rmmod btusb
    sudo rmmod ath3k
    sudo rmmod rfcomm
    sudo rmmod bluetooth
else 
    echo "Unload the UART BT ..."
    sudo rmmod bnep
    sudo rmmod ath3k
    sudo rmmod rfcomm
    sudo rmmod hci_uart
    #sudo rmmod btsdio
    sudo rmmod bluetooth
fi

lsmod |grep cfg80211 && rmmod cfg80211
lsmod |grep compat && rmmod compat

echo " Unloading Done."
sudo hciconfig 
