# Copyright (C) 2011 The Android Open Source Project.
#
# Original code licensed under the Apache License, Version 2.0 (the "License");
# you may not use this software except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# hw/<LIGHTS_HARDWARE_MODULE_ID>.<ro.hardware>.so

include $(CLEAR_VARS)
LOCAL_SRC_FILES := hdmi_cec.cpp
LOCAL_PRELINK_MODULE := false
LOCAL_C_INCLUDES += \
  $(TOP)/$(BOARD_AML_VENDOR_PATH)/frameworks/services/hdmicec/binder \
  libnativehelper/include/nativehelper \
  libnativehelper/include_jni \
  hardware/libhardware/include \
  frameworks/native/libs/binder/include \
  system/core/base/include

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := \
  vendor.amlogic.hardware.hdmicec@1.0 \
  liblog \
  libutils \
  libcutils \
  libhdmicec

LOCAL_MODULE := hdmi_cec.amlogic
LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
