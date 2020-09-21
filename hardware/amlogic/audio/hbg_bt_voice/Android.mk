# Copyright (C) 2019-2020 HAOBO Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
        hbg_blehid_mic.c \
        ringBuffer.c

LOCAL_C_INCLUDES +=                      \
    hardware/libhardware/include \
    $(TOPDIR)system/core/include        \
    $(TOPDIR)system/media/audio/include

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libhbgdecode

LOCAL_MODULE := libhbg

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
    LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_CFLAGS := -Werror -Wall
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
