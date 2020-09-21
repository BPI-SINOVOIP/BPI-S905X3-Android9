LOCAL_PATH:= $(call my-dir)

# the library
# ============================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  $(call all-subdir-java-files)

LOCAL_SRC_FILES += com/droidlogic/tvinput/services/ITvScanService.aidl
LOCAL_SRC_FILES += com/droidlogic/tvinput/services/IUpdateUiCallbackListener.aidl
LOCAL_SRC_FILES += com/droidlogic/tvinput/services/IAudioEffectsService.aidl

LOCAL_MODULE := droidlogic-tv
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_DX_FLAGS := --core-library

LOCAL_JAVA_LIBRARIES := droidlogic

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_JAVA_LIBRARY)

#copy xml to permissions directory
include $(CLEAR_VARS)
LOCAL_MODULE := droidlogic.tv.software.core.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := $(LOCAL_MODULE)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/etc/permissions
else
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
endif

include $(BUILD_PREBUILT)
