#!/bin/bash

echo "s+@BASE@+${BASE}+; s+@APP@+${APP}+; s+@RELPATH@+${RELPATH}+" > ${ROOTDIR}/build/gen.sed

sed -f ${ROOTDIR}/build/gen.sed ${ROOTDIR}/rule/app.in

