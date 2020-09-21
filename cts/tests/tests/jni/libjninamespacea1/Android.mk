# Copyright (C) 2016 The Android Open Source Project
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
# This is the shared library included by the JNI test app.
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libjninamespacea1

# Don't include this package in any configuration by default.
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := namespacea1.cpp

LOCAL_CFLAGS := -Wall -Werror

LOCAL_C_INCLUDES := $(JNI_H_INCLUDE) $(LOCAL_PATH)/../libjnicommon/

LOCAL_LDLIBS += -llog
LOCAL_SHARED_LIBRARIES := libdl liblog libnativehelper_compat_libc++ libjnicommon

LOCAL_SDK_VERSION := 23
LOCAL_NDK_STL_VARIANT := c++_shared

include $(BUILD_SHARED_LIBRARY)
