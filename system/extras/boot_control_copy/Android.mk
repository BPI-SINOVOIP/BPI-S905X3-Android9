# Copyright 2015 The Android Open Source Project

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := boot_control_copy.cpp bootinfo.cpp
LOCAL_CFLAGS := -Wall -Werror -Wno-missing-field-initializers -Wno-unused-parameter
LOCAL_C_INCLUDES := bootable/recovery
LOCAL_HEADER_LIBRARIES := bootimg_headers
LOCAL_SHARED_LIBRARIES := libbase libcutils
LOCAL_STATIC_LIBRARIES := libfs_mgr

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE:= bootctrl.default
include $(BUILD_SHARED_LIBRARY)
