LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := samples

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_SDK_VERSION := current

LOCAL_DEX_PREOPT := false

LOCAL_PACKAGE_NAME := VoiceRecognitionService

include $(BUILD_PACKAGE)
