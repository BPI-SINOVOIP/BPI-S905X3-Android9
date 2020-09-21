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

ifeq ($(LOWPAN_HAL_ENABLED),true)
include $(CLEAR_VARS)
LOCAL_MODULE := lowpan_hdlc_adapter
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    lowpan_hdlc_adapter.cpp \
    hdlc_lite.c

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libdl \
	libbase \
	libutils \
	libhardware

LOCAL_SHARED_LIBRARIES += \
	libhidlbase \
	libhidltransport \
	android.hardware.lowpan@1.0

include $(BUILD_EXECUTABLE)
endif
