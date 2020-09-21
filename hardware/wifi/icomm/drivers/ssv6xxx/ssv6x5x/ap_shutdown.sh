#! /bin/bash

echo "@@Unload hostapd..."
dir=$(pwd)

HOSTPAD_DIR=../hostapd

$HOSTPAD_DIR/unload_ap.sh

echo "@@Unload wireless driver..."
sleep 1

./unload.sh




