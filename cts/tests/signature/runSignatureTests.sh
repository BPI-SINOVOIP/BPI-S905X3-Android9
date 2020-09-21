#! /bin/bash
#
# Copyright 2017 The Android Open Source Project.
#
# Builds and runs signature APK tests.

set -e

if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "Missing environment variables. Did you run build/envsetup.sh and lunch?" >&2
    exit 1
fi

if [ $# -eq 0 ]; then
    PACKAGES="
CtsCurrentApiSignatureTestCases
CtsSystemApiSignatureTestCases
CtsAndroidTestMockCurrentApiSignatureTestCases
CtsAndroidTestRunnerCurrentApiSignatureTestCases
CtsAndroidTestBase27ApiSignatureTestCases

CtsApacheHttpLegacy27ApiSignatureTestCases
CtsApacheHttpLegacyCurrentApiSignatureTestCases
CtsApacheHttpLegacyUsesLibraryApiSignatureTestCases

CtsSystemApiAnnotationTestCases

CtsHiddenApiBlacklistCurrentApiTestCases
CtsHiddenApiBlacklistApi27TestCases
CtsHiddenApiBlacklistDebugClassTestCases

CtsHiddenApiKillswitchWildcardTestCases
CtsHiddenApiKillswitchWhitelistTestCases
CtsHiddenApiKillswitchDebugClassTestCases
"
else
    PACKAGES=${1+"$@"}
fi

cd $ANDROID_BUILD_TOP
make -j32 $PACKAGES

TMPFILE=$(mktemp)
trap "echo Removing temporary directory; rm -f $TMPFILE" EXIT

for p in $PACKAGES
do
    echo cts -a arm64-v8a -m "$p" >> $TMPFILE
done

cts-tradefed run cmdfileAndExit $TMPFILE
