LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
         $(call all-subdir-java-files) \
         com/droidlogic/inputmethod/remote/IPinyinDecoderService.aidl
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := com.droidlogic.inputmethod.remote.lib

include $(BUILD_STATIC_JAVA_LIBRARY)
