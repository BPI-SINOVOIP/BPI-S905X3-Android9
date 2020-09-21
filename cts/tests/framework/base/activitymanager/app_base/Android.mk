LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

# Don't include this package in any target.
LOCAL_MODULE_TAGS := tests

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-test \
    cts-amwm-util \

LOCAL_SRC_FILES := \
    $(call all-java-files-under, src) \
    $(call all-named-files-under,Components.java, ../app) \
    $(call all-named-files-under,Components.java, ../app27) \

LOCAL_MODULE := cts-am-app-base

LOCAL_SDK_VERSION := current

include $(BUILD_STATIC_JAVA_LIBRARY)
