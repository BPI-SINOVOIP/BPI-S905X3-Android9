#!/bin/bash
#
# Copyright 2011 Google Inc. All Rights Reserved.
# Author: raymes@google.com (Raymes Khoury)

# Make sure the base toolchain-utils directory is in our PYTHONPATH before
# trying to run this script.
export PYTHONPATH+=":.."

num_tests=0
num_failed=0

for test in $(find -name \*test.py); do
  echo RUNNING: ${test}
  ((num_tests++))
  if ! ./${test} ; then
    echo
    echo "*** Test Failed! (${test}) ***"
    echo
    ((num_failed++))
  fi
done

echo

if [ ${num_failed} -eq 0 ] ; then
  echo "ALL TESTS PASSED (${num_tests} ran)"
  exit 0
fi

echo "${num_failed} TESTS FAILED (out of ${num_tests})"
exit 1
