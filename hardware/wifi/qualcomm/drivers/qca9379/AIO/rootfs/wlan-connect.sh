SECURITY=$1
SSID=$2
PASSPHASE=$3
TOPDIR=`pwd`

#
# detect the network device
#
if [ "$WLANDEV" = "" -a -e /sys/class/net ]; then
	for dev in `ls /sys/class/net/`; do
		if [ -e /sys/class/net/$dev/device/idProduct ]; then
			PID=`cat /sys/class/net/$dev/device/idProduct`
			if [ "$PID" = "9378" ]; then
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


#SBIN_PATH="${TOPDIR}/WLAN-CAF/rootfs-x86.build/sbin"
WPA_CLI="${TOPDIR}/sbin/wpa_cli -i $WLANDEV"
ifconfig $WLANDEV 192.168.1.4 netmask 255.255.255.0
#echo ${WPA_CLI} 
if [ "${SECURITY}" == "open" ]; then
	echo "=============Set ${SECURITY} Security============="
	${WPA_CLI} remove_network all
	${WPA_CLI} add_network
	${WPA_CLI} disable_network all
	${WPA_CLI} set_network 0 ssid \"${SSID}\"
	${WPA_CLI} set_network 0 priority 0
	${WPA_CLI} set_network 0 key_mgmt NONE
	${WPA_CLI} set_network 0 auth_alg OPEN
	${WPA_CLI} set_network 0 scan_ssid 1
	${WPA_CLI} enable_network all
	${WPA_CLI} reassociate
elif [ "${SECURITY}" == "wpa2" ]; then
	echo "=============Set ${SECURITY} Security============="
	${WPA_CLI} remove_network all
	${WPA_CLI} add_network
	${WPA_CLI} disable_network all
	${WPA_CLI} set_network 0 proto "RSN"
	${WPA_CLI} set_network 0 ssid \"${SSID}\"
	${WPA_CLI} set_network 0 priority 0
	${WPA_CLI} set_network 0 key_mgmt WPA-PSK
	${WPA_CLI} set_network 0 pairwise CCMP
	${WPA_CLI} set_network 0 psk \"${PASSPHASE}\"
	${WPA_CLI} set_network 0 auth_alg OPEN
	${WPA_CLI} set_network 0 scan_ssid 1
	${WPA_CLI} enable_network all
	${WPA_CLI} reassociate
else
	echo "=============Disconnect WLAN============="
	${WPA_CLI} disconnect
fi

