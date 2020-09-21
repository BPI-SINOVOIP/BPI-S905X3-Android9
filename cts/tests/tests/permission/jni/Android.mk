# Copyright (C) 2010 The Android Open Source Project
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

include $(CLEAR_VARS)

LOCAL_MODULE := libctspermission_jni

# Don't include this package in any configuration by default.
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
		CtsPermissionsJniOnLoad.cpp \
		android_permission_cts_FileUtils.cpp

LOCAL_C_INCLUDES := $(JNI_H_INCLUDE) 

LOCAL_SHARED_LIBRARIES := libnativehelper_compat_libc++ liblog
LOCAL_CPPFLAGS := -std=gnu++11
LOCAL_CXX_STL := libc++_static

LOCAL_CFLAGS := -Wno-unused-parameter -Wall -Werror

include $(BUILD_SHARED_LIBRARY)
