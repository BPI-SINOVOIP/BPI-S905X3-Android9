LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(wildcard $(LOCAL_PATH)/../../system/bootaccelerate/Android.mk),$(LOCAL_PATH)/../../system/bootaccelerate/Android.mk)
$(info "have bootaccelerate source code")
else
$(info "no bootaccelerate source code, add prebuilts")
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := accelerateboot
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

endif
