#!/bin/bash
./ver_info.pl include/ssv_firmware_version.h
./ver_info.pl ../../include/ssv_firmware_version.h
make scratch
echo "Done ko!"
