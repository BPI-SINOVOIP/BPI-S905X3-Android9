# Copyright (c) 2014 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.
#
# Description: makefile

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_JAVA_LIBRARIES := droidlogic droidlogic-tv

LOCAL_PACKAGE_NAME := MboxLauncher

LOCAL_OVERRIDES_PACKAGES := Home

LOCAL_PROGUARD_ENABLED := full
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true

FILE := device/*/$(TARGET_PRODUCT)/files/MboxLauncher2/AndroidManifest-common.xml
FILES := $(foreach v,$(wildcard $(FILE)),$(v))

ifeq ($(FILES), $(wildcard $(FILE)))
LOCAL_FULL_LIBS_MANIFEST_FILES := $(FILES)
LOCAL_JACK_COVERAGE_INCLUDE_FILTER := com.droidlogic.mboxlauncher*
endif

include $(BUILD_PACKAGE)
