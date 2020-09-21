#!/bin/bash -u
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script is part of the Android binary search triage process.
# It should be the first script called by the user, after the user has set up
# the two necessary build tree directories (see the prerequisites section of
# README.android).
#
# WARNING:
#   Before running this script make sure you have setup the Android build
#   environment in this shell (i.e. successfully run 'lunch').
#
# This script takes three arguments.  The first argument must be the path of
# the android source tree being tested. The second (optional) argument is the
# device ID for fastboot/adb so the test device can be uniquely indentified in
# case multiple phones are plugged in. The final (optional) argument is the
# number of jobs that various programs can use for parallelism
# (make, xargs, etc.). There is also the argument for bisection directory, but
# this is not strictly an argument for just this script (as it should be set
# during the POPULATE_GOOD and POPULATE_BAD steps, see README.android for
# details).
#
# Example call:
#   ANDROID_SERIAL=002ee16b1558a3d3 NUM_JOBS=10 android/setup.sh ~/android
#
#   This will setup the bisector for Nexus5X, using 10 jobs, where the android
#   source lives at ~/android.
#
# NOTE: ANDROID_SERIAL is actually an option used by ADB. You can also simply
# do 'export ANDROID_SERIAL=<device_id>' and the bisector will still work.
# Furthermore, if your device is the only Android device plugged in you can
# ignore ANDROID_SERIAL.
#
# This script sets all necessary environment variables, and ensures the
# environment for the binary search triage process is setup properly. In
# addition, this script generates common.sh, which generates enviroment
# variables used by the other scripts in the package binary search triage process.
#

#
# Positional arguments
#

ANDROID_SRC=$1

#
# Optional arguments
#

# If DEVICE_ID is not null export this as ANDROID_SERIAL for use by adb
# If DEVICE_ID is null then leave null
DEVICE_ID=${ANDROID_SERIAL:+"export ANDROID_SERIAL=${ANDROID_SERIAL} "}

NUM_JOBS=${NUM_JOBS:-"1"}
BISECT_ANDROID_DIR=${BISECT_DIR:-~/ANDROID_BISECT}

#
# Set up basic variables.
#

GOOD_BUILD=${BISECT_ANDROID_DIR}/good
BAD_BUILD=${BISECT_ANDROID_DIR}/bad
WORK_BUILD=${ANDROID_SRC}

#
# Verify that the necessary directories exist.
#

if [[ ! -d ${GOOD_BUILD} ]] ; then
  echo "Error:  ${GOOD_BUILD} does not exist."
  exit 1
fi

if [[ ! -d ${BAD_BUILD} ]] ; then
  echo "Error:  ${BAD_BUILD} does not exist."
  exit 1
fi

if [[ ! -d ${WORK_BUILD} ]] ; then
  echo "Error:  ${WORK_BUILD} does not exist."
  exit 1
fi

#
# Verify that good/bad object lists are the same
#

good_list=`mktemp`
bad_list=`mktemp`
sort ${GOOD_BUILD}/_LIST > good_list
sort ${BAD_BUILD}/_LIST > bad_list

diff good_list bad_list
diff_result=$?
rm good_list bad_list

if [ ${diff_result} -ne 0 ]; then
  echo "Error: good and bad object lists differ."
  echo "diff exited with non-zero status: ${diff_result}"
  exit 1
fi

#
# Ensure android build environment is setup
#
# ANDROID_PRODUCT_OUT is only set once lunch is successfully executed. Fail if
# ANDROID_PRODUCT_OUT is unset.
#

if [ -z ${ANDROID_PRODUCT_OUT+0} ]; then
  echo "Error: Android build environment is not setup."
  echo "cd to ${ANDROID_SRC} and do the following:"
  echo "  source build/envsetup.sh"
  echo "  lunch <device_lunch_combo>"
  exit 1
fi

#
# Create common.sh file, containing appropriate environment variables.
#

COMMON_FILE="android/common.sh"

cat <<-EOF > ${COMMON_FILE}

BISECT_ANDROID_DIR=${BISECT_ANDROID_DIR}

BISECT_ANDROID_SRC=${ANDROID_SRC}
BISECT_NUM_JOBS=${NUM_JOBS}

BISECT_GOOD_BUILD=${GOOD_BUILD}
BISECT_BAD_BUILD=${BAD_BUILD}
BISECT_WORK_BUILD=${WORK_BUILD}

BISECT_GOOD_SET=${GOOD_BUILD}/_LIST
BISECT_BAD_SET=${BAD_BUILD}/_LIST

${DEVICE_ID}

export BISECT_STAGE="TRIAGE"

EOF

chmod 755 ${COMMON_FILE}

exit 0
