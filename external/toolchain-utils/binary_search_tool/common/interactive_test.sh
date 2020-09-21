#!/bin/bash -u
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script pings the chromebook to determine if it successfully booted.
# It then asks the user if the image is good or not, allowing the user to
# conduct whatever tests the user wishes, and waiting for a response.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on ChromeOS package and object files. It
# waits for the test setup script to build and install the image, then asks the
# user if the image is good or not. It should return '0' if the test succeeds
# (the image is 'good'); '1' if the test fails (the image is 'bad'); and '125'
# if it could not determine (does not apply in this case).
#

source common/common.sh

ping -c 3 -W 3 ${BISECT_REMOTE}
retval=$?

if [[ ${retval} -eq 0 ]]; then
    echo "ChromeOS image has been built and installed on ${BISECT_REMOTE}."
else
    exit 1
fi

while true; do
    read -p "Is this a good ChromeOS image?" yn
    case $yn in
        [Yy]* ) exit 0;;
        [Nn]* ) exit 1;;
        * ) echo "Please answer yes or no.";;
    esac
done

exit 125
