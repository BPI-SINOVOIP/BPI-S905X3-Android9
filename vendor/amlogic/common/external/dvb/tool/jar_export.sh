#!/bin/sh

if [ "x$1" = "x" ];then
	CMD=`basename $0`
	echo "usage: $CMD destination"
	echo "       Copy am_adp.jar&am_mw.jar under build/ to destination"
	exit -1
fi

if [ ! -d $1 ];then
	echo "Destination \"$1\" dose not exist"
	exit -1
fi


cp build/android/android/am_adp/java/am_adp.jar build/android/android/am_mw/java/am_mw.jar $1


