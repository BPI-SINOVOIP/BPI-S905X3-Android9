# Copyright 2010 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

#
# -- All host/targets including windows
#

include $(CLEAR_VARS)
LOCAL_SRC_FILES := blk_alloc_to_base_fs.c
LOCAL_MODULE := blk_alloc_to_base_fs
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_CFLAGS_darwin := -DHOST
LOCAL_CFLAGS_linux := -DHOST
LOCAL_CFLAGS += -Wall -Werror
include $(BUILD_HOST_EXECUTABLE)

#
# -- All host/targets excluding windows
#

ifneq ($(HOST_OS),windows)

include $(CLEAR_VARS)
LOCAL_MODULE := mkuserimg_mke2fs.sh
LOCAL_SRC_FILES := mkuserimg_mke2fs.sh
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_REQUIRED_MODULES := mke2fs e2fsdroid
# We don't need any additional suffix.
LOCAL_MODULE_SUFFIX :=
LOCAL_BUILT_MODULE_STEM := $(notdir $(LOCAL_SRC_FILES))
LOCAL_IS_HOST_MODULE := true
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := mke2fs.conf
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/etc
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := mke2fs.conf
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := mke2fs.conf
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true
include $(BUILD_PREBUILT)

endif
