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

# Common variables and actions for inclusion in patching scripts.

# Fail immediately on error
set -e

if [[ -z ${ANDROID_BUILD_TOP} ]]; then
  echo ANDROID_BUILD_TOP not set
  exit 1
fi

relpath() {
  python -c "import os.path; print os.path.relpath('$1','${2:-$PWD}')" ;
}

ICU_DIR=${ANDROID_BUILD_TOP}/external/icu
ICU4J_DIR=${ICU_DIR}/android_icu4j
SCRIPT_DIR=${ICU_DIR}/tools/srcgen/javadoc_patches
PATCHES_DIR=${ICU_DIR}/tools/srcgen/javadoc_patches/patches
SRCGEN_DIR=${ICU_DIR}/tools/srcgen
