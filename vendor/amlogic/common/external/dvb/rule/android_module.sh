#!/bin/bash

MODULES=

if [ -f Android.mk ]; then
	TMP=`cat Android.mk | grep LOCAL_MODULE | grep -v LOCAL_MODULE_TAG | sed s/LOCAL_MODULE// | sed s/:=//`
	MODULES="$MODULES $TMP"
	TMP=`cat Android.mk | grep LOCAL_PACKAGE_NAME | sed s/LOCAL_PACKAGE_NAME// | sed s/:=//`
	MODULES="$MODULES $TMP"
fi

SUBDIRS=`cat Makefile | grep SUBDIRS | sed s/SUBDIRS// | sed s/:=//`

for i in $SUBDIRS; do
	if [ -d $i ]; then
		cd $i
		TMP=`../$0`
		MODULES="$MODULES $TMP"
		cd ..
	fi
done

echo $MODULES
