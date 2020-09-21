#!/bin/sh

tarfile=ssv_smartlink.tar.gz

tar czvf $tarfile airkiss airkiss.c qqlink qqlink.c ssv_smartlink.c ssv_smartlink.h libssv_smartlink-mipsel.a ssv_smartlink.h Makefile.airkiss Makefile.qqlink Makefile.smarticomm tencent/ Tencent_iot_SDK/ --exclude=".svn" 

echo "======================================="
ls -l $tarfile
echo "======================================="
