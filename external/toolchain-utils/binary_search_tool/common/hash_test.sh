#!/bin/bash -u
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script is intended to be used by binary_search_state.py. It is to
# be used for testing/development of the binary search triage tool
# itself.  It waits for the test setup script to build and install the
# image, then checks the hashes in the provided file.
# If the real hashes match the checksum hashes, then the image is 'good',
# otherwise it is 'bad'.  This allows the rest of the bisecting tool
# to run without requiring help from the user (as it would if we were
# dealing with a real 'bad' image).
#

#
# Initialize the value below before using this script!!!
#
# Make an md5sum of all the files you want to check. For example if you want
# file1, file2, and file3 to be found as bad items:
#
#   md5sum file1 file2 file3 > checksum.out
#
# (Make sure you are hashing the files from your good build and that the hashes
# from good to bad build differ)
#
# Then set HASHES_FILE to be the path to 'checksum.out'
# In this example, file1, file2, file3 will be found as the bad files
# because their hashes won't match when from the bad build tree. This is
# assuming that the hashes between good/bad builds change. It is suggested to
# build good and bad builds at different optimization levels to help ensure
# each item has a different hash.
#
# WARNING:
# Make sure paths to all files are absolute paths or relative to
# binary_search_state.py
#
# cros_pkg bisector example:
#   1. Build good packages with -O1, bad packages with -O2
#   2. cros_pkg/switch_to_good.sh pkg1 pkg2 pkg3
#   3. md5sum pkg1 pkg2 pkg3 > checksum.out.cros_pkg
#   4. Set HASHES_FILE to be checksum.out.cros_pkg
#   5. Run the bisector with this test script
#
#
HASHES_FILE=

if [[ -z "${HASHES_FILE}" || ! -f "${HASHES_FILE}" ]];
then
    echo "ERROR: HASHES_FILE must be intialized in common/hash_test.sh"
    exit 3
fi

md5sum -c --status ${HASHES_FILE}
md5_result=$?


exit $md5_result
