#!/bin/bash -u
#
# Copyright 2015 Google Inc. All Rights Reserved.
#
# This script is part of the ChromeOS package binary search triage process.
# It should be the first script called by the user, after the user has set up
# the three necessary build tree directories (see the prerequisites section of
# README.cros_pkg_triage).
#
# This script requires two arguments.  The first argument must be the name of
# the board for which this work is being done (e.g. 'daisy', 'lumpy','parrot',
# etc.).  The second argument must be the name or IP address of the chromebook
# on which the ChromeOS images will be pushed and tested.
#
# This script sets up a soft link definining /build/${board} to point
# to the working build tree, for the binary search triags process.  In
# addition, this script generates two other scripts, common.sh,
# which generates enviroment variables used by the other scripts in the
# package binary search triage process; and ${board}_cleanup.sh,
# which undoes the various changes that this script performs, returning the
# user's environment to its original state.
#

# Set up basic variables.

BOARD=$1
REMOTE=$2

GOOD_BUILD=/build/${BOARD}.good
BAD_BUILD=/build/${BOARD}.bad
WORK_BUILD=/build/${BOARD}.work

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
# Check to see if /build/${BOARD} already exists and if so, in what state.
# Set appropriate flags & values, in order to be able to undo these changes
# in ${board}_cleanup.sh. If it's a soft link, remove it; if it's a
# read tree, rename it.
#

build_tree_existed=0
build_tree_was_soft_link=0
build_tree_renamed=0
build_tree_link=""

if [[ -d "/build/${BOARD}" ]] ; then
    build_tree_existed=1
    if [[ -L "/build/${BOARD}" ]] ; then
        build_tree_was_soft_link=1
        build_tree_link=`readlink /build/${BOARD}`
        sudo rm /build/${BOARD}
    else
        build_tree_renamed=1
        sudo mv /build/${BOARD} /build/${BOARD}.save
    fi
fi

# Make "working' tree the default board tree (set up soft link)

sudo ln -s /build/${BOARD}.work /build/${BOARD}

#
# Create common.sh file, containing appropriate environment variables.
#

COMMON_FILE="common/common.sh"

cat <<-EOF > ${COMMON_FILE}

BISECT_BOARD=${BOARD}
BISECT_REMOTE=${REMOTE}
BISECT_MODE="PACKAGE_MODE"

GOOD_BUILD=/build/${BOARD}.good
BAD_BUILD=/build/${BOARD}.bad
WORK_BUILD=/build/${BOARD}.work

EOF

chmod 755 ${COMMON_FILE}

#
# Create clean-up script, calling create_cleanup_script.py with
# the appropriate flags.
#

if [[ ${build_tree_existed} -eq 0 ]] ; then

    python cros_pkg/create_cleanup_script.py --board=${BOARD} \
        --old_tree_missing

elif [[ ${build_tree_was_soft_link} -eq 0 ]] ; then

    python cros_pkg/create_cleanup_script.py --board=${BOARD} \
        --renamed_tree

else

    python cros_pkg/create_cleanup_script.py --board=${BOARD} \
        --old_link="'${build_tree_link}'"
fi

chmod 755 cros_pkg/${BOARD}_cleanup.sh

exit 0
