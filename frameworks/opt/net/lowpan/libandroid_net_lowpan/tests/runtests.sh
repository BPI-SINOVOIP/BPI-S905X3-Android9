#!/usr/bin/env bash

if [ -z $ANDROID_BUILD_TOP ]; then
  echo "You need to source and lunch before you can use this script"
  exit 1
fi

echo "Running tests"

set -e # fail early

echo "+ mmma -j32 $ANDROID_BUILD_TOP/frameworks/opt/net/lowpan/libandroid_net_lowpan/tests"
# NOTE Don't actually run the command above since this shell doesn't inherit functions from the
#      caller.
make -j32 -C $ANDROID_BUILD_TOP -f build/core/main.mk MODULES-IN-frameworks-opt-net-lowpan-libandroid_net_lowpan-tests

set -x # print commands

adb root
adb wait-for-device

adb install -r -g "$OUT/data/app/FrameworksLowpanApiNativeTests/FrameworksLowpanApiNativeTests.apk"

adb shell am instrument -w "$@" 'android.net.lowpan.testnative/android.support.test.runner.AndroidJUnitRunner'
