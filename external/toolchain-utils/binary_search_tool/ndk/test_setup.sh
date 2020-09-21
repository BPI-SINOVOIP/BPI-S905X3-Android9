#!/bin/bash
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This is the setup script for generating and installing the ndk app.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on the Android source tree. It should
# return '0' if the setup succeeds; and '1' if the setup fails (the image
# could not build or be flashed).
#

echo
echo "INSTALLATION BEGIN"
echo

# This normally shouldn't be hardcoded, but this is a sample bisection.
# Also keep in mind that the bisection tool mandates all paths are
# relative to binary_search_state.py
cd ndk/Teapot

echo "BUILDING APP"

./gradlew installArm7Debug
gradle_status=$?

exit ${gradle_status}
