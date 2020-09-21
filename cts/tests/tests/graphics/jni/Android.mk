# Copyright 2016 The Android Open Source Project
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

LOCAL_MODULE := libctsgraphics_jni

LOCAL_MODULE_TAGS := tests

LOCAL_SRC_FILES := \
	CtsGraphicsJniOnLoad.cpp \
	android_graphics_cts_ANativeWindowTest.cpp \
	android_graphics_cts_ASurfaceTextureTest.cpp \
	android_graphics_cts_BasicVulkanGpuTest.cpp \
	android_graphics_cts_BitmapTest.cpp \
	android_graphics_cts_SyncTest.cpp \
	android_graphics_cts_CameraGpuCtsActivity.cpp \
	android_graphics_cts_CameraVulkanGpuTest.cpp \
	android_graphics_cts_MediaVulkanGpuTest.cpp \
	android_graphics_cts_VulkanFeaturesTest.cpp \
	android_graphics_cts_VulkanPreTransformCtsActivity.cpp \
	CameraTestHelpers.cpp \
	ImageReaderTestHelpers.cpp \
	MediaTestHelpers.cpp \
	NativeTestHelpers.cpp \
	VulkanPreTransformTestHelpers.cpp \
	VulkanTestHelpers.cpp

LOCAL_CFLAGS += -std=c++14 -Wall -Werror -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_STATIC_LIBRARIES := libvkjson_ndk
LOCAL_SHARED_LIBRARIES := libandroid libvulkan libnativewindow libsync liblog libdl libjnigraphics \
                          libcamera2ndk libmediandk libEGL libGLESv2
LOCAL_NDK_STL_VARIANT := c++_static

LOCAL_SDK_VERSION := current

include $(BUILD_SHARED_LIBRARY)
