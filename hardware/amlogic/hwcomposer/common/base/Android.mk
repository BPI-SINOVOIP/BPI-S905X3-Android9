# Copyright (C) 2008 The Android Open Source Project
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

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_CPPFLAGS := $(HWC_CPP_FLAGS)
LOCAL_CFLAGS := $(HWC_C_FLAGS)
LOCAL_SHARED_LIBRARIES := $(HWC_SHARED_LIBS)

LOCAL_SRC_FILES := \
    DrmTypes.cpp \
    DrmSync.cpp \
    DrmFramebuffer.cpp

LOCAL_C_INCLUDES := \
    system/core/include \
    hardware/amlogic/gralloc/amlogic \
    hardware/libhardware/include \
    system/core/libsync/include \
    $(LOCAL_PATH)/include

LOCAL_STATIC_LIBRARIES := \
    hwc.utils_static

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    hardware/amlogic/gralloc/amlogic \
    $(LOCAL_PATH)/include

LOCAL_MODULE := hwc.base_static

include $(BUILD_STATIC_LIBRARY)
