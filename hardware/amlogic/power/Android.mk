# Copyright (C) 2011 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_C_INCLUDES += \
	hardware/libhardware/include \
	system/core/libcutils/include \
	system/core/libutils/include \
	system/core/libsystem/include \
	frameworks/native/libs/binder/include \
	vendor/amlogic/common/frameworks/services/hdmicec/binder/


LOCAL_MODULE := power.amlogic
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := power.cpp
LOCAL_SHARED_LIBRARIES := \
  liblog \
  libcutils

ifeq ($(strip $(BOARD_HAVE_CEC_HIDL_SERVICE)),true)
    LOCAL_SHARED_LIBRARIES += vendor.amlogic.hardware.hdmicec@1.0 libhdmicec
    LOCAL_CFLAGS += -DBOARD_HAVE_CEC_HIDL_SERVICE
endif
LLOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Wno-unused-parameter

include $(BUILD_SHARED_LIBRARY)
