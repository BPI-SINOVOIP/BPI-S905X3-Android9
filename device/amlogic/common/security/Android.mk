LOCAL_PATH:= $(call my-dir)

#######################################
# verity_key
include $(CLEAR_VARS)

LOCAL_MODULE := verity_key.amlogic
LOCAL_SRC_FILES := verity_key
LOCAL_MODULE_STEM := verity_key
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)

include $(BUILD_PREBUILT)
