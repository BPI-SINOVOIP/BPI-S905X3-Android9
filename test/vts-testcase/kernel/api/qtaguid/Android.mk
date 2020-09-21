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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE := vts_test_binary_qtaguid_module
LOCAL_SRC_FILES := SocketTagUserSpace.cpp
LOCAL_SHARED_LIBRARIES += libutils liblog libbase
LOCAL_STATIC_LIBRARIES += libqtaguid
LOCAL_C_INCLUDES += system/extras/tests/include \
                    test/vts/testcases/system/qtaguid/sample
LOCAL_CFLAGS += \
  -fno-strict-aliasing \
  -Wall \
  -Werror \
  -Wno-unused-variable \

include $(BUILD_NATIVE_TEST)

include $(CLEAR_VARS)

LOCAL_MODULE := VtsKernelQtaguidTest
VTS_CONFIG_SRC_DIR := testcases/kernel/api/qtaguid
-include test/vts/tools/build/Android.host_config.mk
