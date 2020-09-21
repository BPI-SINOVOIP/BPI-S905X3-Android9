#!/bin/sh
#
#    WFA-stop.sh : Stop script for Wi-Fi Direct Testing.
#

#
#    Parameters
#

USER=`whoami`

if [ $USER != "root" ]; then
        echo You must be 'root' to run the command
        exit 1
fi

echo "=============Stop wpa_supplicant..."
killall -q wpa_cli
sleep 1
killall -q wpa_supplicant
sleep 1
killall -q hostapd
sleep 1

#
echo "=============Uninstall Driver..."
rmmod iwldvm 2>/dev/null
rmmod iwlwifi 2>/dev/null
rmmod mac80211 2>/dev/null
rmmod wlan  2> /dev/null
rmmod cfg80211 2> /dev/null
rmmod compat 2> /dev/null
echo "=============Done!"
