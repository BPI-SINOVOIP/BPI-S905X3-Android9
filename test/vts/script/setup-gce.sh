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

adb root
adb push ${ANDROID_BUILD_TOP}/out/target/product/gce_x86/system/bin/vts_hal_driver /data/local/tmp/vts_hal_driver
adb push ${ANDROID_BUILD_TOP}/out/target/product/gce_x86/system/bin/vts_hal_agent /data/local/tmp/vts_hal_agent
adb shell chmod 755 /data/local/tmp/vts_hal_driver
adb shell chmod 755 /data/local/tmp/vts_hal_agent
adb shell /data/local/tmp/vts_hal_agent
