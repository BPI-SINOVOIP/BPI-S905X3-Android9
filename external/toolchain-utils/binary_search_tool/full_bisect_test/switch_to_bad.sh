#!/bin/bash -u
#
# This is one of the scripts that is passed to binary_search_state.py to do
# the bisection.  This one takes a list of object files (either a real list or
# a file containing the list) and copies the files from the bad objects
# directory to the working directory.
#

source full_bisect_test/common.sh

pushd ${BISECT_WORK_BUILD}
chmod 644 *

OBJ_LIST_FILES=$1
FILE_ARGS=0

if [[ -f ${OBJ_LIST_FILES} ]] ; then
  file ${OBJ_LIST_FILES} &> ${BISECT_WORK_BUILD}/file_type.tmp
  grep "ASCII text" ${BISECT_WORK_BUILD}/file_type.tmp
  result=$?
  if [[ ${result} -eq 0 ]] ; then
    FILE_ARGS=1
  fi
  rm ${BISECT_WORK_BUILD}/file_type.tmp
fi

overall_status=0

if [[ ${FILE_ARGS} -eq 1 ]] ; then
  while read obj || [[ -n "${obj}" ]];
  do
    cp ${BISECT_BAD_BUILD}/${obj} ${BISECT_WORK_BUILD}
    status=$?
    if [[ ${status} -ne 0 ]] ; then
      echo "Failed to copy ${obj} to work build tree."
      overall_status=2
    fi
  done < ${OBJ_LIST_FILES}
else

  for o in "$@"
  do
    cp ${BISECT_BAD_BUILD}/${o} ${BISECT_WORK_BUILD}
    status=$?
    if [[ ${status} -ne 0 ]] ; then
      echo "Failed to copy ${o} to work build tree."
      overall_status=2
    fi
  done
fi

popd

exit ${overall_status}
