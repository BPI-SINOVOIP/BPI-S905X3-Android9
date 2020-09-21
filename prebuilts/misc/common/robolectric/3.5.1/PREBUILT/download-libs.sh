#! /bin/bash

set -e

printusage() {
    echo "Usage: ./download-libs.sh <robolectric-version> " >&2
    echo "    -f <old-directory-to-copy-from>" >&2
    exit 1
}

oldVersion=""
roboVersion="$1"
shift

while getopts "f:h" opt; do
    case "$opt" in
        f)
            oldVersion="$OPTARG"
            ;;
        h)
            printusage
            ;;
    esac
done

if [[ -z $roboVersion ]] || [[ -z $oldVersion ]]; then
    printusage
fi

mkdir -p ../"$roboVersion"/PREBUILT
# Copy the scripts into the versioned directory for record
cp download-libs.sh ../"$roboVersion"/PREBUILT/download-libs.sh
cp download-libs.gradle ../"$roboVersion"/PREBUILT/download-libs.gradle

cd ../"$roboVersion"
gradle -b PREBUILT/download-libs.gradle \
    -ProbolectricVersion="$roboVersion" \
    -PshadowsVersion="$roboVersion"

COPY_FROM_OLD_VERSION=(
    "java-timeout"
    "list_failed.sh"
    "report-internal.mk"
    "robotest-internal.mk"
    "robotest.sh"
    "run_robotests.mk"
    "wrapper.sh"
    "wrapper_test.sh"
)

for file in "${COPY_FROM_OLD_VERSION[@]}"; do
    cp -n ../"$oldVersion"/$file ./$file
done

cat <<EOF > Android.mk
LOCAL_PATH:= \$(call my-dir)

############################
# Adding the Robolectric .JAR prebuilts from this directory into a single target.
# This is the one you probably want.
include \$(CLEAR_VARS)

LOCAL_STATIC_JAVA_LIBRARIES := \\
    platform-robolectric-${roboVersion}-annotations \\
    platform-robolectric-${roboVersion}-junit \\
    platform-robolectric-${roboVersion}-multidex \\
    platform-robolectric-${roboVersion}-resources \\
    platform-robolectric-${roboVersion}-sandbox \\
    platform-robolectric-${roboVersion}-shadow-api \\
    platform-robolectric-${roboVersion}-shadows-framework \\
    platform-robolectric-${roboVersion}-shadows-httpclient \\
    platform-robolectric-${roboVersion}-snapshot \\
    platform-robolectric-${roboVersion}-utils

LOCAL_MODULE := platform-robolectric-${roboVersion}-prebuilt

LOCAL_SDK_VERSION := current

include \$(BUILD_STATIC_JAVA_LIBRARY)

############################
# Defining the target names for the static prebuilt .JARs.
include \$(CLEAR_VARS)

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \\
    platform-robolectric-${roboVersion}-annotations:lib/annotations-${roboVersion}.jar \\
    platform-robolectric-${roboVersion}-junit:lib/junit-${roboVersion}.jar \\
    platform-robolectric-${roboVersion}-resources:lib/resources-${roboVersion}.jar \\
    platform-robolectric-${roboVersion}-sandbox:lib/sandbox-${roboVersion}.jar \\
    platform-robolectric-${roboVersion}-shadow-api:lib/shadowapi-${roboVersion}.jar \\
    platform-robolectric-${roboVersion}-snapshot:lib/robolectric-${roboVersion}.jar \\
    platform-robolectric-${roboVersion}-utils:lib/utils-${roboVersion}.jar \\
    platform-robolectric-${roboVersion}-multidex:lib/shadows-multidex-${roboVersion}.jar \\
    platform-robolectric-${roboVersion}-shadows-framework:lib/shadows-framework-${roboVersion}.jar \\
    platform-robolectric-${roboVersion}-shadows-httpclient:lib/shadows-httpclient-${roboVersion}.jar

include \$(BUILD_MULTI_PREBUILT)
EOF

set +e
