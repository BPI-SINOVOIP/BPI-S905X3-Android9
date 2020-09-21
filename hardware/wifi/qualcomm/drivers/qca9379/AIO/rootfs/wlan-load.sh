#!/bin/sh
#
#    WFA-start-usb-single.sh : Start script for Wi-Fi Direct Testing.
#

#
#    Parameters
#
TOPDIR=`pwd`

MODULE_PATH=${TOPDIR}/lib/modules

WPA_SUPPLICANT=${TOPDIR}/sbin/wpa_supplicant
WPA_CLI=${TOPDIR}/sbin/wpa_cli
SIGMA_DUT=${TOPDIR}/sbin/sigma_dut
IW=${TOPDIR}/sbin/iw

WFA_SCRIPTS_PATH=${TOPDIR}/home/atheros/Atheros-P2P/scripts
P2P_ACT_FILE=${WFA_SCRIPTS_PATH}/p2p-action.sh
P2P_DEV_CONF=${WFA_SCRIPTS_PATH}/p2pdev_dual.conf
WLAN_ACT_FILE=${WFA_SCRIPTS_PATH}/wlan-action.sh
WLAN_DEV_CONF=${WFA_SCRIPTS_PATH}/empty.conf
WPA_SUPPLICANT_ENTROPY_FILE=${WFA_SCRIPTS_PATH}/entropy.dat

ETHDEV=eth0
WLANPHY=
WLANDEV=
P2PDEV=p2p0

#
# some sanity checking
#
USER=`whoami`
if [ $USER != "root" ]; then
        echo You must be 'root' to run the command
        exit 1
fi

#
# detect the device existence
# Right now we assume this notebook has only one mmc bus

DEVICE_USB=`lsusb | grep "0cf3:9378"`
DEVICE_PCI=`lspci | grep "Atheros Communications Inc. Device 003e (rev 30)"`
DEVICE_PCI1=`lspci | grep "Qualcomm Atheros Device 003e (rev 30)"`
DEVICE_SDIO=`dmesg | grep "SDIO"`
if [ "$DEVICE_PCI" = "" -a "$DEVICE_PCI1" = "" -a "$DEVICE_USB" = "" -a "$DEVICE_SDIO" = "" ]; then
	echo You must insert device before running the command
	exit 2
fi

# disable rfkill
rfkill unblock all

#
# install driver
#
echo "=============Install Driver..."
insmod $MODULE_PATH/compat.ko
#insmod $MODULE_PATH/compat_firmware_class.ko 2> /dev/null
insmod $MODULE_PATH/cfg80211.ko
insmod $MODULE_PATH/wlan.ko
sleep 3

#
# detect the network device
#
if [ "$WLANDEV" = "" -a -e /sys/class/net ]; then
	for dev in `ls /sys/class/net/`; do
		if [ -e /sys/class/net/$dev/device/idProduct ]; then
			USB_PID=`cat /sys/class/net/$dev/device/idProduct`
			if [ "$USB_PID" = "9378" ]; then
				WLANDEV=$dev
			fi
		fi
		if [ -e /sys/class/net/$dev/device/device ]; then
			PCI_DID=`cat /sys/class/net/$dev/device/device`
			if [ "$PCI_DID" = "0x003e" ]; then
				WLANDEV=$dev
			fi
		fi
		if [ -e /sys/class/net/$dev/device/device ]; then
			SDIO_DID=`cat /sys/class/net/$dev/device/device`
			if [  "$SDIO_DID" = "0x0509" ] || [ "$SDIO_DID" = "0x0504" ]; then
				WLANDEV=$dev
			fi
		fi
		if [ -e /sys/class/net/$dev/phy80211/name ]; then
			WLANPHY=`cat /sys/class/net/$dev/phy80211/name`
		fi
	done
	if [ "$WLANDEV" = "" ]; then
		echo Fail to detect wlan device
		exit 3
	fi
fi


if [ "$WLANDEV" = "" ]; then
	WLANDEV=wlan0
	WLANPHY=phy0
fi

#${IW} dev ${WLANDEV} interface add ${P2PDEV} type managed

sleep 1
#iwconfig $WLANDEV power off
#iwconfig $P2PDEV  power off

#
# wlan device detected and configure ethernet
#
echo WLAN_DEV:$WLANDEV 
echo P2P_DEV:$P2PDEV 
echo ETH_device: $ETHDEV
#ifconfig $ETHDEV 192.168.250.40

#
#    Start wpa_supplicant
#
echo "=============Start wpa_supplicant..."
echo "Start Command : ${WPA_SUPPLICANT} -Dnl80211 -i ${P2PDEV} -c ${P2P_DEV_CONF} -N -Dnl80211 -i ${WLANDEV} -c ${WLAN_DEV_CONF} -e ${WPA_SUPPLICANT_ENTROPY_FILE}"
${WPA_SUPPLICANT} -Dnl80211 -i ${P2PDEV} -c ${P2P_DEV_CONF} -N -Dnl80211 -i ${WLANDEV} -c ${WLAN_DEV_CONF} -e ${WPA_SUPPLICANT_ENTROPY_FILE} &
sleep 1

#
#    Other configuraiton 
#
echo "Setting System Configuration........."
tcp_userconfig=`grep -c tcp_use_userconfig /etc/sysctl.conf`

echo "Setting System Configuration........."
tcp_userconfig=`grep -c tcp_use_userconfig /etc/sysctl.conf`

if [ $tcp_userconfig -eq 0 ]
then
sudo echo "net.ipv4.tcp_use_userconfig = 1" >> /etc/sysctl.conf
sudo echo "net.ipv4.tcp_delack_seg = 10" >> /etc/sysctl.conf
fi

sudo echo 1 > /proc/sys/net/ipv4/tcp_use_userconfig
sudo echo 10 > /proc/sys/net/ipv4/tcp_delack_seg

echo "=============Done!"
