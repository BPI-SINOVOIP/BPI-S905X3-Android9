#!/bin/bash -u
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script is part of the ChromeOS object binary search triage process.
# It should be the first script called by the user, after the user has set up
# the two necessary build tree directories (see sysroot_wrapper/README).
#
# This script requires three arguments.  The first argument must be the name of
# the board for which this work is being done (e.g. 'daisy', 'lumpy','parrot',
# etc.).  The second argument must be the name or IP address of the chromebook
# on which the ChromeOS images will be pushed and tested. The third argument
# must be the name of the package being bisected (e.g. 'chromeos-chrome',
# 'cryptohome', etc.).
#
# This script generates common/common.sh, which generates enviroment variables
# used by the other scripts in the object file binary search triage process.
#

# Set up basic variables.
bisect_dir=${BISECT_DIR:-/tmp/sysroot_bisect}

BOARD=$1
REMOTE=$2
PACKAGE=$3

GOOD_BUILD=${bisect_dir}/good
BAD_BUILD=${bisect_dir}/bad
GOOD_LIST=${GOOD_BUILD}/_LIST
BAD_LIST=${BAD_BUILD}/_LIST

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

if [[ ! -e ${GOOD_LIST} ]] ; then
    echo "Error:  ${GOOD_LIST} does not exist."
    exit 1
fi

if [[ ! -e ${BAD_LIST} ]] ; then
    echo "Error: ${BAD_LIST} does not exist."
    exit 1
fi

COMMON_FILE="common/common.sh"

cat <<-EOF > ${COMMON_FILE}

BISECT_BOARD=${BOARD}
BISECT_REMOTE=${REMOTE}
BISECT_PACKAGE=${PACKAGE}
BISECT_MODE="OBJECT_MODE"

bisect_dir=${bisect_dir}

export BISECT_STAGE=TRIAGE

EOF

chmod 755 ${COMMON_FILE}

exit 0
