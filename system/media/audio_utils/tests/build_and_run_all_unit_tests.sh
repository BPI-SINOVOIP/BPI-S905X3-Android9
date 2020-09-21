#!/bin/bash
#
# Run tests in this directory.
#

if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "Android build environment not set"
    exit -1
fi

# ensure we have mm
. $ANDROID_BUILD_TOP/build/envsetup.sh

mm

echo "waiting for device"

adb root && adb wait-for-device remount

echo "========================================"
echo "testing primitives"
adb push $OUT/system/lib/libaudioutils.so /system/lib
adb push $OUT/data/nativetest/primitives_tests/primitives_tests /system/bin
adb shell /system/bin/primitives_tests

echo "testing power"
adb push $OUT/data/nativetest/power_tests/power_tests /system/bin
adb shell /system/bin/power_tests

echo "testing channels"
adb push $OUT/data/nativetest/channels_tests/channels_tests /system/bin
adb shell /system/bin/channels_tests

echo "string test"
adb push $OUT/data/nativetest/string_tests/string_tests /system/bin
adb shell /system/bin/string_tests

echo "format tests"
adb push $OUT/data/nativetest/format_tests/format_tests /system/bin
adb shell /system/bin/format_tests

echo "benchmarking primitives"
adb push $OUT/system/bin/primitives_benchmark /system/bin
adb shell /system/bin/primitives_benchmark
