#
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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := vts_shell_driver
LOCAL_MODULE_STEM_64 := vts_shell_driver64
LOCAL_MODULE_STEM_32 := vts_shell_driver32
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  ShellDriver.cpp \
  ShellDriverMain.cpp \

LOCAL_SHARED_LIBRARIES := \
  libbase \
  libutils \
  libcutils \
  libvts_multidevice_proto \
  libprotobuf-cpp-full \
  libvts_drivercomm \

LOCAL_C_INCLUDES += \
  test/vts/proto \
  test/vts/drivers/libdrivercomm \
  external/protobuf/src \

LOCAL_CFLAGS := -Werror -Wall

LOCAL_MULTILIB := both

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := vts_shell_driver_test
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  ShellDriver.cpp \
  ShellDriverTest.cpp \

LOCAL_SHARED_LIBRARIES := \
  libbase \
  libutils \
  libcutils \
  libvts_multidevice_proto \
  libprotobuf-cpp-full \
  libvts_drivercomm \

LOCAL_C_INCLUDES += \
  test/vts/proto \
  test/vts/drivers/libdrivercomm \
  external/protobuf/src \

LOCAL_CFLAGS := -Werror -Wall

include $(BUILD_NATIVE_TEST)
