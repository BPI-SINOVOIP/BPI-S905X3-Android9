#!/bin/bash
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This is a generic ChromeOS package/image test setup script. It is meant to
# be used for either the object file or package bisection tools. This script
# does one of the following depending on what ${BISECT_MODE} is set to:
#
# 1) ${BISECT_MODE} is PACKAGE_MODE:
#   build_image is called and generates a new ChromeOS image using whatever
#   packages are currently in the build tree. This image is then pushed to the
#   remote machine using flash over ethernet (or usb flash if ethernet flash
#   fails).
#
# 2) ${BISECT_MODE} is OBJECT_MODE:
#   emerge is called for ${BISECT_PACKAGE} and generates a build for said
#   package. This package is then deployed to the remote machine and the machine
#   is rebooted. If deploying fails then a new ChromeOS image is built from
#   scratch and pushed to the machine like in PACKAGE_MODE.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on ChromeOS objects and packages. It should
# return '0' if the setup succeeds; and '1' if the setup fails (the image
# could not build or be flashed).
#

export PYTHONUNBUFFERED=1

source common/common.sh

usb_flash()
{
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
        [Yy]*) return 0;;
        [Nn]*) return 1;;
        *) echo "Please answer y or n.";;
    esac
  done
}

ethernet_flash()
{
  echo
  echo "Please ensure your Chromebook is up and running Chrome so"
  echo "cros flash may run."
  echo "If your Chromebook has a broken image you can try:"
  echo "1. Rebooting your Chromebook 6 times to install the last working image"
  echo "2. Alternatively, running the following command on the Chromebook"
  echo "   will also rollback to the last working image:"
  echo "   'update_engine_client --rollback --nopowerwash --reboot'"
  echo "3. Flashing a new image through USB"
  echo
  sleep 1
  read -p $'Press enter to continue and retry the ethernet flash' notused
  cros flash --board=${BISECT_BOARD} --clobber-stateful ${BISECT_REMOTE} ~/trunk/src/build/images/${BISECT_BOARD}/latest/chromiumos_test_image.bin
}

reboot()
{
  ret_val=0
  pushd ~/trunk/src/scripts > /dev/null
  set -- --remote=${BISECT_REMOTE}
  . ./common.sh || ret_val=1
  . ./remote_access.sh || ret_val=1
  TMP=$(mktemp -d)
  FLAGS "$@" || ret_val=1
  remote_access_init || ret_val=1
  remote_reboot || ret_val=1
  popd > /dev/null

  return $ret_val
}

echo
echo "INSTALLATION BEGIN"
echo

if [[ "${BISECT_MODE}" == "OBJECT_MODE" ]]; then
  echo "EMERGING ${BISECT_PACKAGE}"
  CLEAN_DELAY=0 emerge-${BISECT_BOARD} -C ${BISECT_PACKAGE}
  emerge-${BISECT_BOARD} ${BISECT_PACKAGE}
  emerge_status=$?

  if [[ ${emerge_status} -ne 0 ]] ; then
    echo "emerging ${BISECT_PACKAGE} returned a non-zero status: $emerge_status"
    exit 1
  fi

  echo
  echo "DEPLOYING"
  echo "cros deploy ${BISECT_REMOTE} ${BISECT_PACKAGE}"
  cros deploy ${BISECT_REMOTE} ${BISECT_PACKAGE} --log-level=info
  deploy_status=$?

  if [[ ${deploy_status} -eq 0 ]] ; then
    echo "Deploy successful. Rebooting device..."
    reboot
    if [[ $? -ne 0 ]] ; then
      echo
      echo "Could not automatically reboot device!"
      read -p "Please manually reboot device and press enter to continue" notused
    fi
    exit 0
  fi

  echo "Deploy failed! Trying build_image/cros flash instead..."
  echo
fi

echo "BUILDING IMAGE"
pushd ~/trunk/src/scripts
./build_image test --board=${BISECT_BOARD} --noenable_rootfs_verification --noeclean
build_status=$?
popd

if [[ ${build_status} -eq 0 ]] ; then
    echo
    echo "FLASHING"
    echo "Pushing built image onto device."
    echo "cros flash --board=${BISECT_BOARD} --clobber-stateful ${BISECT_REMOTE} ~/trunk/src/build/images/${BISECT_BOARD}/latest/chromiumos_test_image.bin"
    cros flash --board=${BISECT_BOARD} --clobber-stateful ${BISECT_REMOTE} ~/trunk/src/build/images/${BISECT_BOARD}/latest/chromiumos_test_image.bin
    cros_flash_status=$?
    while [[ ${cros_flash_status} -ne 0 ]] ; do
        while true; do
          echo
          echo "cros flash has failed! From here you can:"
          echo "1. Flash through USB"
          echo "2. Retry flashing over ethernet"
          echo "3. Abort this installation and skip this image"
          echo "4. Abort this installation and mark test as failed"
          sleep 1
          read -p "Which method would you like to do? " choice
          case $choice in
              1) usb_flash && break;;
              2) ethernet_flash && break;;
              3) exit 125;;
              4) exit 1;;
              *) echo "Please answer 1, 2, 3, or 4.";;
          esac
        done

        cros_flash_status=$?
    done
else
    echo "build_image returned a non-zero status: ${build_status}"
    exit 1
fi

exit 0
