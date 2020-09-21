#!/bin/bash
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This is a generic ChromeOS package/image test setup script. It is meant to
# be used for the package bisection tool, in particular when there is a booting
# issue with the image, so the image MUST be 'flashed' via USB.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on ChromeOS objects and packages. It should
# return '0' if the setup succeeds; and '1' if the setup fails (the image
# could not built or be flashed).
#

export PYTHONUNBUFFERED=1

source common/common.sh

echo "BUILDING IMAGE"
pushd ~/trunk/src/scripts
./build_image test --board=${BISECT_BOARD} --noenable_rootfs_verification --noeclean
build_status=$?
popd

if [[ ${build_status} -eq 0 ]] ; then
    echo
    echo "INSTALLING IMAGE VIA USB (requires some manual intervention)"
    echo
    echo "Insert a usb stick into the current machine"
    echo "Note: The cros flash will take time and doesn't give much output."
    echo "      Be patient. If your usb access light is flashing it's working."
    sleep 1
    read -p "Press enter to continue" notused

    cros flash --board=${BISECT_BOARD} --clobber-stateful usb:// ~/trunk/src/build/images/${BISECT_BOARD}/latest/chromiumos_test_image.bin

    echo
    echo "Flash to usb complete!"
    echo "Plug the usb into your chromebook and install the image."
    echo "Refer to the ChromiumOS Developer's Handbook for more details."
    echo "http://www.chromium.org/chromium-os/developer-guide#TOC-Boot-from-your-USB-disk"
    while true; do
      sleep 1
      read -p "Was the installation of the image successful? " choice
      case $choice in
        [Yy]*) exit 0;;
        [Nn]*) exit 1;;
        *) echo "Please answer y or n.";;
      esac
    done
else
    echo "build_image returned a non-zero status: ${build_status}"
    exit 1
fi

exit 0
