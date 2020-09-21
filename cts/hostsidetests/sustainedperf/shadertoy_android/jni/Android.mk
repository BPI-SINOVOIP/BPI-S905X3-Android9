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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libgltest

LOCAL_SRC_FILES := shaders.cpp \
                   shadertoy_renderer.cpp \
                   shadertoy_shader.cpp \
                   hooks_android.cpp \
                   utils.cpp

LOCAL_CFLAGS := -Wall -Werror -Wno-unused-variable

LOCAL_SDK_VERSION := 21
LOCAL_NDK_STL_VARIANT := c++_static

LOCAL_LDLIBS    := -llog -lGLESv3 -lEGL -ldl
include $(BUILD_SHARED_LIBRARY)
