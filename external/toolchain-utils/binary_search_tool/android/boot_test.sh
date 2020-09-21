#!/bin/bash -u
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script pings the android device to determine if it successfully booted.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on the Android source tree. It
# waits for the test setup script to build and install the image, then checks
# if image boots or not. It should return '0' if the test succeeds
# (the image is 'good'); '1' if the test fails (the image is 'bad'); and '125'
# if it could not determine (does not apply in this case).
#

source android/common.sh

# Check if boot animation has stopped and trim whitespace
is_booted()
{
  # Wait for boot animation to stop and trim whitespace
  status=`adb shell getprop init.svc.bootanim | tr -d '[:space:]'`
  [[ "$status" == "stopped" ]]
  return $?
}

# Wait for device to boot, retry every 1s
# WARNING: Do not run without timeout command, could run forever
wait_for_boot()
{
  while ! is_booted
  do
    sleep 1
  done
}

echo "Waiting 60 seconds for device to come online..."
timeout 60 adb wait-for-device
retval=$?

if [[ ${retval} -eq 0 ]]; then
  echo "Android image has been built and installed."
else
  echo "Device failed to reboot within 60 seconds."
  exit 1
fi

echo "Waiting 60 seconds for device to finish boot..."
# Spawn subshell that will timeout in 60 seconds
# Feed to cat so that timeout will recognize it as a command
# (timeout only works for commands/programs, not functions)
timeout 60 cat <(wait_for_boot)
retval=$?

if [[ ${retval} -eq 0 ]]; then
  echo "Android device fully booted!"
else
  echo "Device failed to fully boot within 60 seconds."
  exit 1
fi

exit ${retval}
