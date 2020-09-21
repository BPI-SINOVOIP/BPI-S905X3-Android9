# Copyright © 2010, 2013
#    Thorsten Glaser <t.glaser@tarent.de>
# This file is provided under the same terms as mksh.

LOCAL_PATH := $(call my-dir)

# /system/etc/mkshrc

include $(CLEAR_VARS)

LOCAL_MODULE := mkshrc
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# /vendor/etc/mkshrc
include $(CLEAR_VARS)

LOCAL_MODULE := mkshrc_vendor
LOCAL_MODULE_STEM := mkshrc
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_ETC)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)
