#!/usr/bin/env bash

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

# This script is called by git diff with GIT_EXTERNAL_DIFF

# git calls this script with 7 parameters:
# path old-file old-hex old-mode new-file new-hex new-mode
# if 8th parameter is specified, the change is written incrementally

source $(dirname ${BASH_SOURCE})/common.sh

OLD_FILE=${2}
NEW_FILE=${5}
INCREMENTAL=${8}
echo "diffing ${NEW_FILE}"
# Compute the patch file path
TARGET_FILE=$PWD/${NEW_FILE}
PATCH_FILE=${PATCHES_DIR}/$(relpath ${TARGET_FILE} ${ICU4J_DIR})".patch"

# Create the dst directory
mkdir -p $(dirname ${PATCH_FILE})

# Write the diff into the patch file
# Replace the tmp file path in the first line with the real source path
if [ -z ${INCREMENTAL} ]; then
  diff -u "${NEW_FILE}" "${OLD_FILE}" | sed "2s#${OLD_FILE}#${NEW_FILE}#" > ${PATCH_FILE}
else
  diff -u "${OLD_FILE}" "${NEW_FILE}" | sed "1s#${OLD_FILE}#${NEW_FILE}#" >> ${PATCH_FILE}
fi

