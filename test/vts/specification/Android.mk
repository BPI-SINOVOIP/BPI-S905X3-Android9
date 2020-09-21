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

vtslib_interfacespec_srcfiles := \
  lib/ndk/bionic/1.0/libmV1.vts \
  lib/ndk/bionic/1.0/libcV1.vts \
  lib/ndk/bionic/1.0/libcutilsV1.vts \

vtslib_interfacespec_includes := \
  $(LOCAL_PATH) \
  test/vts/drivers/hal \
  test/vts/drivers/hal/common \
  test/vts/drivers/hal/framework \
  test/vts/drivers/hal/libdatatype \
  test/vts/drivers/hal/libmeasurement \
  bionic \
  libcore \
  device/google/gce/include \
  system/extras \
  system/media/camera/include \
  external/protobuf/src \
  external/libedit/src \
  $(TARGET_OUT_HEADERS) \

vtslib_interfacespec_shared_libraries := \
  libbase \
  libcutils \
  liblog \
  libdl \
  libandroid_runtime \
  libcamera_metadata \
  libvts_datatype \
  libvts_common \
  libvts_measurement \
  libvts_multidevice_proto \
  libprotobuf-cpp-full \

vtslib_interfacespec_static_libraries := \
  libbluetooth-types

include $(CLEAR_VARS)

# libvts_interfacespecification does not include or link any HIDL HAL driver.
# HIDL HAL drivers and profilers are defined as separated shared libraries
# in a respective hardware/interfaces/<hal name>/<version>/Android.bp file.
# libvts_interfacespecification is the driver for:
#   legacy HALs,
#   conventional HALs,
#   shared libraries,
#   and so on.

LOCAL_MODULE := libvts_interfacespecification
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  ${vtslib_interfacespec_srcfiles} \

LOCAL_C_INCLUDES := \
  ${vtslib_interfacespec_includes} \
  system/core/base/include \

LOCAL_SHARED_LIBRARIES := \
  ${vtslib_interfacespec_shared_libraries} \

LOCAL_STATIC_LIBRARIES := \
  ${vtslib_interfacespec_static_libraries}

LOCAL_CFLAGS := \
  -Wall \
  -Werror \

# These warnings are in code generated with vtsc
# b/31362043
LOCAL_CFLAGS += \
  -Wno-unused-parameter \
  -Wno-unused-value \
  -Wno-duplicate-decl-specifier \

LOCAL_PROTOC_OPTIMIZE_TYPE := full

LOCAL_MULTILIB := both

include $(BUILD_SHARED_LIBRARY)
