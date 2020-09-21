#!/bin/bash

# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# A simple helper script that runs all of the atest unit tests.
# We have 2 situations we take care of:
#   1. User wants to invoke this script by itself.
#   2. PREUPLOAD hook invokes this script.

ATEST_DIR=`dirname $0`/
PREUPLOAD_FILES=$@
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

function set_pythonpath() {
  local path_to_check=`realpath $ATEST_DIR`
  if ! echo $PYTHONPATH | grep -q $path_to_check; then
    PYTHONPATH=$path_to_check:$PYTHONPATH
  fi
}

function run_atest_unittests() {
  set_pythonpath
  local rc=0
  for test_file in $(find $ATEST_DIR -name "*_unittest.py");
  do
    if ! $test_file; then
      rc=1
    fi
  done

  echo
  if [[ $rc -eq 0 ]]; then
    echo -e "${GREEN}All unittests pass${NC}!"
  else
    echo -e "${RED}There was a unittest failure${NC}"
  fi
  return $rc
}

# Let's check if anything is passed in, if not we assume the user is invoking
# script, but if we get a list of files, assume it's the PREUPLOAD hook.
if [[ -z $PREUPLOAD_FILES ]]; then
  run_atest_unittests
  exit $?
else
  for f in $PREUPLOAD_FILES;
  do
    # We only want to run this unittest if atest files have been touched.
    if [[ $f == atest/* ]]; then
      run_atest_unittests
      exit $?
    fi
  done
fi
