#
# Copyright (C) 2018 The Android Open Source Project
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

FRAMEWORK_MANIFEST_INPUT_FILES := $(LOCAL_PATH)/manifest.xml
ifdef DEVICE_FRAMEWORK_MANIFEST_FILE
  FRAMEWORK_MANIFEST_INPUT_FILES += $(DEVICE_FRAMEWORK_MANIFEST_FILE)
endif

# VNDK Version in device compatibility matrix and framework manifest
ifeq ($(BOARD_VNDK_VERSION),current)
VINTF_VNDK_VERSION := $(PLATFORM_VNDK_VERSION)
else
VINTF_VNDK_VERSION := $(BOARD_VNDK_VERSION)
endif

# Device Compatibility Matrix
ifdef DEVICE_MATRIX_FILE
DEVICE_MATRIX_INPUT_FILE := $(DEVICE_MATRIX_FILE)
else
DEVICE_MATRIX_INPUT_FILE := $(LOCAL_PATH)/device_compatibility_matrix.default.xml
endif

include $(CLEAR_VARS)
LOCAL_MODULE        := device_compatibility_matrix.xml
LOCAL_MODULE_STEM   := compatibility_matrix.xml
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_PATH   := $(TARGET_OUT_VENDOR)/etc/vintf

GEN := $(local-generated-sources-dir)/compatibility_matrix.xml

$(GEN): PRIVATE_VINTF_VNDK_VERSION := $(VINTF_VNDK_VERSION)
$(GEN): $(DEVICE_MATRIX_INPUT_FILE) $(HOST_OUT_EXECUTABLES)/assemble_vintf
	REQUIRED_VNDK_VERSION=$(PRIVATE_VINTF_VNDK_VERSION) \
	BOARD_SYSTEMSDK_VERSIONS="$(BOARD_SYSTEMSDK_VERSIONS)" \
		$(HOST_OUT_EXECUTABLES)/assemble_vintf -i $< -o $@

LOCAL_PREBUILT_MODULE_FILE := $(GEN)
include $(BUILD_PREBUILT)
BUILT_VENDOR_MATRIX := $(LOCAL_BUILT_MODULE)

# Framework Manifest
include $(CLEAR_VARS)
LOCAL_MODULE        := framework_manifest.xml
LOCAL_MODULE_STEM   := manifest.xml
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_PATH   := $(TARGET_OUT)/etc/vintf

GEN := $(local-generated-sources-dir)/manifest.xml

$(GEN): PRIVATE_FLAGS :=

ifeq ($(PRODUCT_ENFORCE_VINTF_MANIFEST),true)
ifdef BUILT_VENDOR_MATRIX
$(GEN): $(BUILT_VENDOR_MATRIX)
$(GEN): PRIVATE_FLAGS += -c "$(BUILT_VENDOR_MATRIX)"
endif
endif

$(GEN): PRIVATE_VINTF_VNDK_VERSION := $(VINTF_VNDK_VERSION)
$(GEN): PRIVATE_FRAMEWORK_MANIFEST_INPUT_FILES := $(FRAMEWORK_MANIFEST_INPUT_FILES)
$(GEN): $(FRAMEWORK_MANIFEST_INPUT_FILES) $(HOST_OUT_EXECUTABLES)/assemble_vintf
	PROVIDED_VNDK_VERSIONS="$(PRIVATE_VINTF_VNDK_VERSION) $(PRODUCT_EXTRA_VNDK_VERSIONS)" \
	PLATFORM_SYSTEMSDK_VERSIONS="$(PLATFORM_SYSTEMSDK_VERSIONS)" \
		$(HOST_OUT_EXECUTABLES)/assemble_vintf \
		-i $(call normalize-path-list,$(PRIVATE_FRAMEWORK_MANIFEST_INPUT_FILES)) \
		-o $@ $(PRIVATE_FLAGS)

LOCAL_PREBUILT_MODULE_FILE := $(GEN)
include $(BUILD_PREBUILT)
BUILT_SYSTEM_MANIFEST := $(LOCAL_BUILT_MODULE)

VINTF_VNDK_VERSION :=
FRAMEWORK_MANIFEST_INPUT_FILES :=
DEVICE_MATRIX_INPUT_FILE :=
