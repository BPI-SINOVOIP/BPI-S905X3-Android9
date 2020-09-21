# Copyright (C) 2012 The Android Open Source Project
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

LOCAL_MODULE    := libctsopenglperf_jni

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := OpenGlPerfNativeJni.cpp

LOCAL_C_INCLUDES := $(JNI_H_INCLUDE) $(call include-path-for, system-core) frameworks/native/opengl/include

LOCAL_SHARED_LIBRARIES := libnativehelper_compat_libc++ liblog libEGL

LOCAL_SDK_VERSION := 14

LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter

include $(BUILD_SHARED_LIBRARY)
