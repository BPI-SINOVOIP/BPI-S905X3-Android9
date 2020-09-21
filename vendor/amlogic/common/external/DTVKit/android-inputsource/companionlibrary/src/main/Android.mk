LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := companionlibrary

LOCAL_AAPT_FLAGS := --auto-add-overlay \
   --extra-packages android.support.v7.appcompat

LOCAL_RESOURCE_DIR := frameworks/support/v7/appcompat/res

LOCAL_STATIC_JAVA_LIBRARIES += \
   android-support-v4 \
   android-support-v7-appcompat

LOCAL_SRC_FILES := $(call all-subdir-java-files)

include $(BUILD_STATIC_JAVA_LIBRARY)

