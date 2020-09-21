# Copyright (C) 2018 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)

# TODO(b/73133405): Currently, building cc_test against NDK using Android.bp
# doesn't work well. Migrate to use Android.bp once b/73133405 gets fixed.

include $(CLEAR_VARS)
LOCAL_MODULE:= dvr_buffer_queue-test

# Includes the dvr_api.h header. Tests should only include "dvr_api.h",
# and shall only get access to |dvrGetApi|, as other symbols are hidden from the
# library.
LOCAL_C_INCLUDES := \
    frameworks/native/libs/vr/libdvr/include \

LOCAL_SANITIZE := thread

LOCAL_SRC_FILES := dvr_buffer_queue-test.cpp

LOCAL_SHARED_LIBRARIES := \
    libandroid \
    liblog \

LOCAL_CFLAGS := \
    -DTRACE=0 \
    -O2 \
    -g \

# DTS Should only link to NDK libraries.
LOCAL_SDK_VERSION := 26
LOCAL_NDK_STL_VARIANT := c++_static

include $(BUILD_NATIVE_TEST)


include $(CLEAR_VARS)
LOCAL_MODULE:= dvr_display-test

LOCAL_C_INCLUDES := \
    frameworks/native/libs/vr/libdvr/include \
    frameworks/native/libs/nativewindow/include

LOCAL_SANITIZE := thread

LOCAL_SRC_FILES := dvr_display-test.cpp

LOCAL_SHARED_LIBRARIES := \
    libandroid \
    liblog

LOCAL_CFLAGS := \
    -DTRACE=0 \
    -O2 \
    -g

# DTS Should only link to NDK libraries.
LOCAL_SDK_VERSION := 26
LOCAL_NDK_STL_VARIANT := c++_static

include $(BUILD_NATIVE_TEST)