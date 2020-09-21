#!/bin/sh

if [ "x$1" = "x" ];then
	CMD=`basename $0`
	echo "usage: $CMD destination"
	echo "       Copy lib*.so under build/ to destination."
	echo "press \"y\" to confirm , other key to cancel."
	exit -1;
fi

if [ ! -d $1 ];then
	echo "Destination \"$1\" does not exsits. "
	exit -1
fi

echo "lib*.so under build/ followed will be copied to $1"

find build/ -name lib*.so -type f -print0 | xargs -0 -n1  | xargs -0 -p | xargs -i -n1 cp {} $1 


