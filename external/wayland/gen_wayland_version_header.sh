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

if [[ $# -ne 1 ]]; then
  echo "Usage: gen_wayland_version_header.sh <path_to_configure.ac>\
    < wayland_version.h.in > wayland_version.h" >&2
  exit 1
fi

set -e
CONFIGURE_AC="$1"
WAYLAND_VERSION_MAJOR="$(grep -o -E 'define..wayland_major_version.+' ${CONFIGURE_AC} | grep -o -E '[0-9]+')"
WAYLAND_VERSION_MINOR="$(grep -o -E 'define..wayland_minor_version.+' ${CONFIGURE_AC} | grep -o -E '[0-9]+')"
WAYLAND_VERSION_MICRO="$(grep -o -E 'define..wayland_micro_version.+' ${CONFIGURE_AC} | grep -o -E '[0-9]+')"
WAYLAND_VERSION="${WAYLAND_VERSION_MAJOR}.${WAYLAND_VERSION_MINOR}.${WAYLAND_VERSION_MICRO}" ; \
sed \
  -e s/@WAYLAND_VERSION_MAJOR@/${WAYLAND_VERSION_MAJOR}/ \
  -e s/@WAYLAND_VERSION_MINOR@/${WAYLAND_VERSION_MINOR}/ \
  -e s/@WAYLAND_VERSION_MICRO@/${WAYLAND_VERSION_MICRO}/ \
  -e s/@WAYLAND_VERSION_MICRO@/${WAYLAND_VERSION_MICRO}/ \
  -e s/@WAYLAND_VERSION@/${WAYLAND_VERSION}/
