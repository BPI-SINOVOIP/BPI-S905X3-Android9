#!/bin/bash
#
# Copyright 2016 The Android Open Source Project
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

if [ -z "$ANDROID_BUILD_TOP" ]; then
  echo "Please run the lunch command first."
elif [ -z "$VTS_PYPI_PATH" ]; then
  echo "Please set environment variable VTS_PYPI_PATH as a new directory to host all PyPI packages."
else
  echo "Local PyPI packages directory is set to" $VTS_PYPI_PATH
  echo "Making local dir" $VTS_PYPI_PATH

  if [ -d "$VTS_PYPI_PATH" ]; then
    echo $VTS_PYPI_PATH "already exists"
  else
    mkdir $VTS_PYPI_PATH -p
  fi

  echo "Downloading PyPI packages to" $VTS_PYPI_PATH
  pip download -d $VTS_PYPI_PATH -r $ANDROID_BUILD_TOP/test/vts/script/pip_requirements.txt --no-binary protobuf,grpcio,numpy,Pillow,scipy
  # The --no-binary option is necessary for packages that have a
  # "-cp27-cp27mu-manylinux1_x86_64.whl" version that will be downloaded instead
  # if --no-binary option is not specified. This version is not compatible with
  # the python virtualenv set up by VtsPythonVirtualenvPreparer.

  # TODO(jaeshin): b/38371975, fix matplotlib download error
  pip download -d $VTS_PYPI_PATH matplotlib --no-binary matplotlib,numpy
fi
