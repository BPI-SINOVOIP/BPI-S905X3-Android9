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
# This is the shared library included by the JNI test app.
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libnativetesthelper_jni

LOCAL_SRC_FILES := \
        gtest_wrapper.cpp

LOCAL_SHARED_LIBRARIES := libnativehelper_compat_libc++
LOCAL_WHOLE_STATIC_LIBRARIES := libgtest_ndk_c++
LOCAL_EXPORT_STATIC_LIBRARY_HEADERS := libgtest_ndk_c++
LOCAL_SDK_VERSION := current
LOCAL_NDK_STL_VARIANT := c++_static
LOCAL_CFLAGS := -Wall -Werror
LOCAL_MULTILIB := both

include $(BUILD_STATIC_LIBRARY)
