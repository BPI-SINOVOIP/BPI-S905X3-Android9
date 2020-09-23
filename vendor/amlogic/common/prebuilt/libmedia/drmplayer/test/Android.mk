ifneq (,$(filter true,$(BOARD_BUILD_VMX_DRM) $(BUILD_WITH_WIDEVINECAS)))
ifeq (,$(wildcard vendor/amlogic/common/frameworks/av/drmplayer))
LOCAL_PATH := $(call my-dir)

#####################################################################
#drmptest
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := drmptest
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_32_BIT_ONLY := true
include $(BUILD_PREBUILT)
endif
endif
