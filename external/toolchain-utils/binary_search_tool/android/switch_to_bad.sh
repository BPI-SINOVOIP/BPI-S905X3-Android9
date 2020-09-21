#!/bin/bash -u
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on the Android source tree.  This script
# symlinks a list of object files from the 'bad' build tree into the working
# build tree, for testing.
#
# It is highly recommended to not use --noincremental with these scripts. If the
# switch scripts are given non incremental sets of GOOD/BAD objects, make will
# not be able to do an incremental build and will take much longer to build.
#


source android/common.sh

OBJ_LIST_FILE=$1

# Symlink from BAD obj to working tree.
SWITCH_CMD="ln -f ${BISECT_BAD_BUILD}/{} {}; touch {};"

overall_status=0

# Check that number of arguments == 1
if [ $# -eq 1 ] ; then
  # Run symlink once per input line, ignore empty lines.
  # Have ${BISECT_NUM_JOBS} processes running concurrently.
  # Pass to "sh" to allow multiple commands to be executed.
  xargs -P ${BISECT_NUM_JOBS} -a ${OBJ_LIST_FILE} -r -l -I '{}' \
    sh -c "${SWITCH_CMD}"
else
  echo "ERROR:"
  echo "Please run the binary search tool with --file_args"
  echo "Android has too many files to be passed as command line arguments"
  echo "The binary search tool will now exit..."
  exit 1
fi
overall_status=$?


exit ${overall_status}
