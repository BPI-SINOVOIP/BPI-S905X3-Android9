LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# Include all common java files.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_MODULE := tv-common
LOCAL_MODULE_CLASS := STATIC_JAVA_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := system_current

LOCAL_USE_AAPT2 := true

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res

LOCAL_JAVA_LIBRARIES := \
    android-support-annotations

LOCAL_DISABLE_RESOLVE_SUPPORT_LIBRARIES := true

LOCAL_SHARED_ANDROID_LIBRARIES := \
    android-support-compat \
    android-support-core-ui \
    android-support-v7-recyclerview \
    android-support-v17-leanback


include $(LOCAL_PATH)/buildconfig.mk

include $(BUILD_STATIC_JAVA_LIBRARY)
