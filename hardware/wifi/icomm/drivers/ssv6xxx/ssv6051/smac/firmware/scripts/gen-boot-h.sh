#!/bin/sh

if [ $# -ne 1 ] ; then
    exit 1
fi

BOOT_TYPE=$(cat $1 | grep -e '^[[:blank:]]*\*[[:blank:]]*BOOT-TYPE:' | awk '{ print $3 }' )

echo "#define _"$BOOT_TYPE"_"

exit 0
