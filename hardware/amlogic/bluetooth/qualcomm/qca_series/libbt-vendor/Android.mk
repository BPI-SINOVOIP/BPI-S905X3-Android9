#
# Copyright 2012 The Android Open Source Project
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

BDROID_DIR := $(TOP_DIR)system/bt
$(info hello qca test)
ifneq ($(BLUETOOTH_INF), USB)
LOCAL_SRC_FILES := \
        src/bt_vendor_qcom.c \
        src/hardware.c \
        src/hci_uart.c \
        src/hci_smd.c \
        src/hw_rome.c \
        src/hw_ar3k.c \
        src/bt_vendor_persist.cpp \
		src/FallthroughBTA.cpp
else
$(info "USE QCOM Bluetooth vendor lib")
LOCAL_SRC_FILES := \
		src/bt_vendor_csr.c \
		src/FallthroughBTA.cpp
endif

ifeq ($(QCOM_BT_USE_SIBS),true)
LOCAL_CFLAGS += -DQCOM_BT_SIBS_ENABLE
endif

ifeq ($(BOARD_HAS_QCA_BT_ROME),true)
LOCAL_CFLAGS += -DBT_SOC_TYPE_ROME
endif

ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
LOCAL_CFLAGS += -DPANIC_ON_SOC_CRASH
endif

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        system/bt/hci/include



LOCAL_SHARED_LIBRARIES := \
        libcutils \
		libutils \
        liblog

LOCAL_MODULE := libbt-vendor
#LOCAL_CLANG := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := qcom
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)

include $(BUILD_SHARED_LIBRARY)

