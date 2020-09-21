#!/bin/bash

if [ ${BASE} = "." ]; then
	BASE=".."
else
	BASE="../${BASE}"
fi

DIR=`dirname $DIR`

while [ $DIR != "." ]; do
	BASE=${BASE}/..
	DIR=`dirname $DIR`
done

echo "s+@BASE@+${BASE}+" > ${ROOTDIR}/build/gen.sed

sed -f ${ROOTDIR}/build/gen.sed ${ROOTDIR}/rule/Makefile.in
