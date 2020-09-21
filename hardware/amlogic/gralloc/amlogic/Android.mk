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
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SHARED_LIBRARIES := liblog libcutils
#Android O sdk version is 26
MESON_GR_USE_BUFFER_USAGE := $(shell expr $(PLATFORM_SDK_VERSION) \> 25)

ifeq ($(MESON_GR_USE_BUFFER_USAGE),1)
LOCAL_SHARED_LIBRARIES += android.hardware.graphics.common@1.0
endif

LOCAL_CFLAGS := -DUSE_BUFFER_USAGE=$(MESON_GR_USE_BUFFER_USAGE)

LOCAL_C_INCLUDES := \
	system/core/libutils/include \
	hardware/libhardware/include \
	$(LOCAL_PATH)/..

LOCAL_SRC_FILES := \
	am_gralloc_ext.cpp
LOCAL_CFLAGS += -DLOG_TAG=\"gralloc\"
LOCAL_MODULE := libamgralloc_ext

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -lt 26 && echo OK),OK)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
#LOCAL_PROPRIETARY_MODULE := true
LOCAL_SHARED_LIBRARIES := liblog libcutils
#Android O sdk version is 26
MESON_GR_USE_BUFFER_USAGE := $(shell expr $(PLATFORM_SDK_VERSION) \> 25)

ifeq ($(MESON_GR_USE_BUFFER_USAGE),1)
LOCAL_SHARED_LIBRARIES += android.hardware.graphics.common@1.0
endif
LOCAL_CFLAGS := -DUSE_BUFFER_USAGE=$(MESON_GR_USE_BUFFER_USAGE)

LOCAL_C_INCLUDES := \
	system/core/libutils/include \
	hardware/libhardware/include \
	$(LOCAL_PATH)/..

LOCAL_SRC_FILES := \
	am_gralloc_ext.cpp
LOCAL_MODULE := libamgralloc_ext_static
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SHARED_LIBRARIES := liblog libcutils
#Android O sdk version is 26
MESON_GR_USE_BUFFER_USAGE := $(shell expr $(PLATFORM_SDK_VERSION) \> 25)

LOCAL_CFLAGS := -DUSE_BUFFER_USAGE=$(MESON_GR_USE_BUFFER_USAGE)

LOCAL_C_INCLUDES := \
	hardware/libhardware/include \
	system/core/libutils/include \
	system/core/libsystem/include \
	$(LOCAL_PATH)/..

LOCAL_SRC_FILES := \
	am_gralloc_internal.cpp
LOCAL_MODULE := libamgralloc_internal_static
include $(BUILD_STATIC_LIBRARY)
