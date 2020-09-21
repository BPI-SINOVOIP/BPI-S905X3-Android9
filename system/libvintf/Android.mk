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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libvintf_recovery
LOCAL_SRC_FILES := VintfObjectRecovery.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/vintf
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_CFLAGS := -Wall -Werror
LOCAL_STATIC_LIBRARIES := \
    libbase \
    libvintf \
    libhidl-gen-utils \
    libfs_mgr

LOCAL_EXPORT_STATIC_LIBRARY_HEADERS := libhidl-gen-utils

include $(BUILD_STATIC_LIBRARY)

$(call dist-for-goals,dist_files,$(HOST_OUT_EXECUTABLES)/checkvintf)
