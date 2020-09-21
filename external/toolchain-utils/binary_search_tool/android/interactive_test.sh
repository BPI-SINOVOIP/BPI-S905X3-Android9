#!/bin/bash -u
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script pings the android device to determine if it successfully booted.
# It then asks the user if the image is good or not, allowing the user to
# conduct whatever tests the user wishes, and waiting for a response.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on the Android source tree. It
# waits for the test setup script to build and install the image, then asks the
# user if the image is good or not. It should return '0' if the test succeeds
# (the image is 'good'); '1' if the test fails (the image is 'bad'); and '125'
# if it could not determine (does not apply in this case).
#

source android/common.sh

echo "Waiting 60 seconds for device to boot..."
timeout 60 adb wait-for-device
retval=$?

if [[ ${retval} -eq 0 ]]; then
  echo "Android image has been built and installed."
else
  echo "Device failed to reboot within 60 seconds."
  exit 1
fi

while true; do
  read -p "Is this a good Android image?" yn
  case $yn in
    [Yy]* ) exit 0;;
    [Nn]* ) exit 1;;
    * ) echo "Please answer yes or no.";;
  esac
done

exit 125
