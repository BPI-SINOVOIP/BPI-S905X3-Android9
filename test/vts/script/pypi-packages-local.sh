#!/bin/bash
#
# Copyright 2017 The Android Open Source Project
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

if [ -z "$ANDROID_BUILD_TOP" ]; then
  echo "Please run the lunch command first."
else
  echo "Downloading PyPI packages"
  sudo pip install -r $ANDROID_BUILD_TOP/test/vts/script/pip_requirements.txt
  sudo pip install matplotlib # TODO(jaeshin): b/38371975, downloaded separately in download-pypi-packages.sh
fi
