# Copyright 2018 The Android Open Source Project
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
LOCAL_MODULE := libahardwarebuffertest
LOCAL_MULTILIB := both

LOCAL_SRC_FILES := \
    AHardwareBufferGLTest.cpp \
    AHardwareBufferTest.cpp \
    GTestMain.cpp

LOCAL_SHARED_LIBRARIES := libandroid liblog libEGL libGLESv2 libGLESv3

LOCAL_WHOLE_STATIC_LIBRARIES := libnativetesthelper_jni

LOCAL_CFLAGS := -Werror -Wall -Wextra -DEGL_EGLEXT_PROTOTYPES -DGL_GLEXT_PROTOTYPES

LOCAL_SDK_VERSION := current
LOCAL_NDK_STL_VARIANT := c++_static

include $(BUILD_SHARED_LIBRARY)
