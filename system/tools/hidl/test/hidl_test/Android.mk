#
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
#
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := hidl_test_helper
LOCAL_MODULE_CLASS := NATIVE_TESTS
LOCAL_SRC_FILES := hidl_test_helper

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := hidl_test
LOCAL_MODULE_CLASS := NATIVE_TESTS
LOCAL_SRC_FILES := hidl_test

LOCAL_REQUIRED_MODULES := \
    hidl_test_client \
    hidl_test_helper \
    hidl_test_servers

LOCAL_REQUIRED_MODULES_arm64 := hidl_test_servers_32 hidl_test_client_32
LOCAL_REQUIRED_MODULES_mips64 := hidl_test_servers_32 hidl_test_client_32
LOCAL_REQUIRED_MODULES_x86_64 := hidl_test_servers_32 hidl_test_client_32

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE := VtsHidlUnitTests
VTS_CONFIG_SRC_DIR := system/tools/hidl/tests
-include test/vts/tools/build/Android.host_config.mk
