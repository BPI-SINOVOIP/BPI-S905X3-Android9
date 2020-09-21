LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := droidlogic-dtvkit
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_DX_FLAGS := --core-library
LOCAL_PRODUCT_MODULE := true

include $(BUILD_JAVA_LIBRARY)

#copy xml to permissions directory
include $(CLEAR_VARS)
LOCAL_MODULE := droidlogic.dtvkit.software.core.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := $(LOCAL_MODULE)

LOCAL_MODULE_PATH := $(TARGET_OUT_PRODUCT)/etc/permissions
LOCAL_PRODUCT_MODULE := true

include $(BUILD_PREBUILT)