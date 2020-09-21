#!/bin/bash

echo ---------------------------Building BT
IF_TYPE='USB'
CUR_DIR=`pwd`
APP_TOPDIR=${CUR_DIR}/../apps
DRIVER_TOPDIR=${CUR_DIR}/../drivers

echo building libhardware
cd ${APP_TOPDIR}/bt_workspace/hardware/libhardware/
bash ./bootstrap
./configure
make
cd ${APP_TOPDIR}/bt_workspace/hardware/libhardware/.libs/
cp libhardware.* ${APP_TOPDIR}/bt_workspace/qcom-opensource/bt/bt-app/main/
cp libhardware.* ${APP_TOPDIR}/bt_workspace/system/bt/main/
cp libhardware.* ${APP_TOPDIR}/bt_workspace/system/bt/test/bluedroidtest/
cp libhardware.* /usr/lib/

echo building libbt-vendor
cd ${APP_TOPDIR}/bt_workspace/hardware/qcom/bt/libbt-vendor/
bash ./bootstrap
./configure
make clean
if [ ${IF_TYPE} == "USB" ]
then
    make
else
    make CFLAGS=-DBT_SOC_TYPE_ROME
fi
cd ${APP_TOPDIR}/bt_workspace/hardware/qcom/bt/libbt-vendor/.libs/
cp libbt-vendor.* /usr/lib/

echo building fluoride
cd ${APP_TOPDIR}/bt_workspace/system/bt/
bash ./bootstrap
./configure
make clean
if [ ${IF_TYPE} == "USB" ]
then
    make CFLAGS=-DBT_SOC_TYPE_ROME_USB
else
    make CFLAGS=-DBT_SOC_TYPE_ROME
fi
cd ${APP_TOPDIR}/bt_workspace/system/bt/main/.libs/
cp libbluetoothdefault.* /usr/lib/
cd ${APP_TOPDIR}/bt_workspace/system/bt/audio_a2dp_hw/.libs/
cp libaudioa2dpdefault.* /usr/lib/

echo Building btapps
cd ${APP_TOPDIR}/bt_workspace/qcom-opensource/bt/gatt/
bash ./bootstrap
./configure 
make

cd ${APP_TOPDIR}/bt_workspace/qcom-opensource/bt/property-ops/
bash ./bootstrap
./configure 
make

cd ${APP_TOPDIR}/bt_workspace/qcom-opensource/bt/bt-app/
bash ./bootstrap
./configure --disable-shared
make

if [ ${IF_TYPE} != "USB" ]
then
echo Building wcnss filter
cd ${APP_TOPDIR}/bt_workspace/vendor/qcom-proprietary/ship/bt/wcnss_filter
bash ./bootstrap
./configure
make
sudo cp wcnssfilter /usr/bin/wcnssfilter
chmod 777 /usr/bin/wcnssfilter
fi

echo Copying bt conf files
cp ${APP_TOPDIR}/bt_workspace/system/bt/conf/bt_stack.conf /etc/bluetooth/bt_stack.conf
cp ${APP_TOPDIR}/bt_workspace/qcom-opensource/bt/bt-app/conf/bt_app.conf /etc/bluetooth/bt_app.conf

echo ---------------------------Done Building BT
