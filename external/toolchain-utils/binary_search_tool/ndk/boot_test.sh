#!/bin/bash -u
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script checks the android device to determine if the app is currently
# running. For our specific test case we will be checking if the Teapot app
# has crashed.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on the Android NDK apps. It
# waits for the test setup script to build and install the app, then checks if
# app boots or not. It should return '0' if the test succeeds
# (the image is 'good'); '1' if the test fails (the image is 'bad'); and '125'
# if it could not determine (does not apply in this case).
#

echo "Starting Teapot app..."
adb shell am start -n com.sample.teapot/com.sample.teapot.TeapotNativeActivity
sleep 3

echo "Checking if Teapot app crashed..."
adb shell ps | grep com.sample.teapot

retval=$?


exit ${retval}
