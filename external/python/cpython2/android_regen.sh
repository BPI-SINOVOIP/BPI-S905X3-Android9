#!/bin/bash -ex
#
# Copyright 2017 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Regenerate host configuration files for the current host

cd `dirname ${BASH_SOURCE[0]}`

DIR=`uname | tr 'A-Z' 'a-z'`_x86_64
mkdir -p $DIR/pyconfig $DIR/libffi
cd $DIR

# Generate pyconfig.h
rm -rf tmp
mkdir tmp
cd tmp
../../configure
cp pyconfig.h ../pyconfig/
cd ..
rm -rf tmp

# Generate ffi.h / fficonfig.h
rm -rf tmp
mkdir tmp
cd tmp
../../Modules/_ctypes/libffi/configure
cp fficonfig.h include/ffi.h ../libffi/
cd ..
rm -rf tmp
