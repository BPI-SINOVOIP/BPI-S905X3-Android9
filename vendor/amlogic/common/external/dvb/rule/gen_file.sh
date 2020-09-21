#!/bin/bash

source ${ROOTDIR}/config/user_info

DATE=`date +%F`

echo "s+@AUTHOR@+${AUTHOR}+; s+@DATE@+${DATE}+; s+@EMAIL@+${EMAIL}+" > ${ROOTDIR}/build/gen.sed

sed -f ${ROOTDIR}/build/gen.sed ${ROOTDIR}/rule/${TYPE}.in
