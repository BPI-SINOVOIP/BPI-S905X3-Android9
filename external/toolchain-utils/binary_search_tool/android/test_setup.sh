#!/bin/bash
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This is the test setup script for generating an Android image based off the
# current working build tree. make is called to relink the object files and
# generate the new Android image to be flashed. The device is then rebooted into
# bootloader mode and fastboot is used to flash the new image. The device is
# then rebooted so the user's test script can run.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on the Android source tree. It should
# return '0' if the setup succeeds; and '1' if the setup fails (the image
# could not build or be flashed).
#

source android/common.sh

manual_flash()
{
  echo
  echo "Please manually flash the built image to your device."
  echo "To do so follow these steps:"
  echo "  1. Boot your device into fastboot mode."
  echo "  2. cd to '${BISECT_ANDROID_SRC}'"
  echo "  2. Run 'source build/envsetup.sh'"
  echo "  3. Run 'lunch'"
  echo "  4. Run '${ADB_DEVICE}fastboot flashall -w'"
  echo "Or see the following link for more in depth steps:"
  echo "https://source.android.com/source/running.html"
  echo
  while true; do
    sleep 1
    read -p "Was the flashing of the image successful? " choice
    case $choice in
      [Yy]*) return 0;;
      [Nn]*) return 1;;
      *) echo "Please answer y or n.";;
    esac
  done
}

auto_flash()
{
  echo
  echo "Please ensure your Android device is on and in fastboot mode so"
  echo "fastboot flash may run."
  echo
  sleep 1
  read -p $'Press enter to continue and retry the flashing' notused

  echo "  ${ADB_DEVICE}fastboot flashall -w"
  fastboot flashall -w
}

flash()
{
  echo
  echo "FLASHING"
  echo "Rebooting device into fastboot mode."
  echo "  ${ADB_DEVICE}adb reboot bootloader"
  adb reboot bootloader

  echo
  echo "Waiting for device to reach fastboot mode."
  echo "(will timeout after 60 seconds)"
  # fastboot will block indefinitely until device comes online.
  # Grab random variable to test if device is online.
  # If takes >60s then we error out and ask the user for help.
  timeout 60 fastboot getvar 0 2>/dev/null
  fastboot_flash_status=$?

  if [[ ${fastboot_flash_status} -eq 0 ]]; then
    echo
    echo "Flashing image."
    echo "  ${ADB_DEVICE}fastboot flashall -w"
    fastboot flashall -w
    fastboot_flash_status=$?
  fi

  while [[ ${fastboot_flash_status} -ne 0 ]] ; do
    echo
    echo "fastboot flash has failed! From here you can:"
    echo "1. Debug and/or flash manually"
    echo "2. Retry flashing automatically"
    echo "3. Abort this installation and skip this image"
    echo "4. Abort this installation and mark test as failed"
    sleep 1
    read -p "Which method would you like to do? " choice
    case $choice in
      1) manual_flash && break;;
      2) auto_flash && break;;
      3) return 125;;
      4) return 1;;
      *) echo "Please answer 1, 2, 3, or 4.";;
    esac
  done
}

# Number of jobs make will use. Can be customized and played with.
MAKE_JOBS=${BISECT_NUM_JOBS}

# Set ADB_DEVICE to "ANDROID_SERIAL=${ANDROID_SERIAL}" or "" if device id not
# set. This is used for debugging info so users can confirm which device
# commands are being sent to.
ADB_DEVICE=${ANDROID_SERIAL:+"ANDROID_SERIAL=${ANDROID_SERIAL} "}

echo
echo "INSTALLATION BEGIN"
echo

cd ${BISECT_ANDROID_SRC}

echo "BUILDING IMAGE"

make -j ${MAKE_JOBS}
make_status=$?

exit_val=0
if [[ ${make_status} -eq 0 ]]; then
  flash
  exit_val=$?
else
  echo "ERROR:"
  echo "make returned a non-zero status: ${make_status}. Skipping image..."
  exit_val=1
fi


exit ${exit_val}
