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

LOCAL_PATH := $(call my-dir)

KEYMASTER_TA_BINARY := 8efb1e1c-37e5-4326-a5d68c33726c7d57

include $(CLEAR_VARS)
LOCAL_MODULE := keystore.amlogic
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := module.cpp \
		   aml_keymaster_ipc.cpp \
		   aml_keymaster_device.cpp \

LOCAL_C_INCLUDES := \
    system/security/keystore \
    $(LOCAL_PATH)/include \
    system/keymaster/ \
    system/keymaster/include \
    external/boringssl/include \
	hardware/libhardware/include \
	system/core/libcutils/include \
	system/core/libsystem/include \
	system/core/libutils/include \
    $(BOARD_AML_VENDOR_PATH)/tdk/ca_export_arm/include \

LOCAL_CFLAGS = -fvisibility=hidden -Wall -Werror
LOCAL_CFLAGS += -DANDROID_BUILD
ifeq ($(USE_SOFT_KEYSTORE), false)
LOCAL_CFLAGS += -DUSE_HW_KEYMASTER
endif
LOCAL_SHARED_LIBRARIES := libcrypto \
			  liblog \
			  libkeymaster_messages \
			  libteec

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -le 26 && echo OK),OK)
LOCAL_SHARED_LIBRARIES += libkeymaster1
endif

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_REQUIRED_MODULES := $(KEYMASTER_TA_BINARY)
include $(BUILD_SHARED_LIBRARY)

#####################################################
#	TA Library
#####################################################
include $(CLEAR_VARS)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := $(KEYMASTER_TA_BINARY)
LOCAL_MODULE_SUFFIX := .ta
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/teetz
ifeq ($(TARGET_ENABLE_TA_SIGN), true)
$(info $(shell mkdir -p $(shell pwd)/$(PRODUCT_OUT)/signed/keymaster))
$(info $(shell $(shell pwd)/$(BOARD_AML_VENDOR_PATH)/tdk/ta_export/scripts/sign_ta_auto.py \
		--in=$(shell pwd)/$(LOCAL_PATH)/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX) \
		--out=$(shell pwd)/$(PRODUCT_OUT)/signed/keymaster/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX) \
		--keydir=$(shell pwd)/$(BOARD_AML_TDK_KEY_PATH)))
LOCAL_SRC_FILES := ../../../$(PRODUCT_OUT)/signed/keymaster/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
else
LOCAL_SRC_FILES := $(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
endif
include $(BUILD_PREBUILT)

