#!/bin/sh

# Copyright 2012 The Android Open Source Project
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

adb reboot fastboot
fastboot flashing unlock_critical
fastboot flashing unlock
fastboot flash bootloader bootloader.img
fastboot flash bootloader-boot0 bootloader.img
fastboot flash bootloader-boot1 bootloader.img
fastboot erase env
fastboot reboot-bootloader
sleep 5
fastboot flashing unlock_critical
fastboot flashing unlock
fastboot flash dts dt.img
fastboot flash dtbo dtbo.img
fastboot -w
fastboot erase param
fastboot erase tee
fastboot flash vbmeta vbmeta.img
fastboot flash odm odm.img
fastboot flash logo logo.img
fastboot flash boot boot.img
fastboot flash system system.img
fastboot flash vendor vendor.img
fastboot flash recovery recovery.img
fastboot flash product product.img
fastboot flashing lock_critical
fastboot flashing lock
fastboot reboot
