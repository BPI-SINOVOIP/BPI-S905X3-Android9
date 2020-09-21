# -*- mode: makefile -*-

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under,src/main/java)
LOCAL_SDK_VERSION := core_current
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := sqlite-jdbc
include $(BUILD_STATIC_JAVA_LIBRARY)

ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under,src/main/java)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := sqlite-jdbc-host
include $(BUILD_HOST_DALVIK_STATIC_JAVA_LIBRARY)
endif  # HOST_OS == linux.
