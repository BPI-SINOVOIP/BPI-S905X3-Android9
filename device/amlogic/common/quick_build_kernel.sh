#!/bin/bash

function usage()
{
    echo "Usage:"
    echo "   Please run the script in android top directory"
    echo "   $0 bootimage      --> build uImage"
    echo "   $0 recoveryimage  --> build recovery uImage"
    echo "   $0 menuconfig     --> kernel menuconfig"
    echo "   $0 savedefconfig  --> save kernel defconfig for commit"
    echo "   $0 build-modules  --> build projects to ko modules"
	exit
}

if [ "$OUT" == "" -o "$TARGET_PRODUCT" == "" ]; then
    echo Please source envsetup.sh and select lunch.
    exit 1
fi
if [ "$PRODUCT_OUT" == "" ]; then
    export PRODUCT_OUT=out/target/product/$TARGET_PRODUCT
fi
if [ "$TARGET_OUT_INTERMEDIATES" == "" ]; then
    export TARGET_OUT_INTERMEDIATES=$PRODUCT_OUT/obj
fi
if [ "$TARGET_OUT" == "" ]; then
    export TARGET_OUT=$PRODUCT_OUT/system
fi


if [ $# -eq 0 ]; then
    usage;
fi

if [ "$1" != "bootimage" ] && [ "$1" != "recoveryimage" ] \
	&& [ "$1" != "menuconfig" ] && [ "$1" != "savedefconfig" ] \
	&& [ "$1" != "build-modules" ]; then
    usage;
fi

if [ "$1" == "bootimage" ]; then
    make -f device/amlogic/$TARGET_PRODUCT/Kernel.mk -j6 bootimage-quick
fi

if [ "$1" == "recoveryimage" ]; then
    make -f device/amlogic/$TARGET_PRODUCT/Kernel.mk -j6 recoveryimage-quick
fi

if [ "$1" == "menuconfig" ]; then
    make -f device/amlogic/$TARGET_PRODUCT/Kernel.mk kernelconfig
fi

if [ "$1" == "savedefconfig" ]; then
    make -i -f device/amlogic/$TARGET_PRODUCT/Kernel.mk savekernelconfig
fi

if [ "$1" == "build-modules" ]; then
	    make -f device/amlogic/$TARGET_PRODUCT/Kernel.mk -j8 build-modules-quick
fi
