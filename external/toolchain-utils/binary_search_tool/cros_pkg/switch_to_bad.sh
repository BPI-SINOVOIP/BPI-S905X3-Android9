#!/bin/bash -u
#
# Copyright 2015 Google Inc. All Rights Reserved.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on ChromeOS packages.  This script
# copies a list of packages from the 'bad' build tree into the working
# build tree, for testing.
#

source common/common.sh

pushd ${WORK_BUILD}

PKG_LIST_FILE=$1

overall_status=0

if [[ -f ${PKG_LIST_FILE} ]] ; then

  # Read every line, and handle case where last line has no newline
  while read pkg || [[ -n "$pkg" ]];
  do
    sudo cp ${BAD_BUILD}/packages/$pkg ${WORK_BUILD}/packages/$pkg
    status=$?
    if [[ ${status} -ne 0 ]] ; then
      echo "Failed to copy ${pkg} to work build tree."
      overall_status=2
    fi
  done < ${PKG_LIST_FILE}
else

  for o in "$@"
  do
    sudo cp ${BAD_BUILD}/packages/$o  ${WORK_BUILD}/packages/$o
    status=$?
    if [[ ${status} -ne 0 ]] ; then
      echo "Failed to copy ${pkg} to work build tree."
      overall_status=2
    fi
  done
fi

popd

exit ${overall_status}
