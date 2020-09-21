#! /bin/bash

echo "@@Unload hostapd..."
dir=$(pwd)

HOSTPAD_DIR=../hostapd/hostapd/

#cd $HOSTPAD_DIR
#$HOSTPAD_DIR/unload_ap.sh
./load_dhcp.sh 

echo "@@Unload wireless driver..."
sleep 1
#cd $dir
./unload.sh




