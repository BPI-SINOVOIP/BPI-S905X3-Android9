LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(wildcard $(LOCAL_PATH)/../external/exoplayer/Android.mk),$(LOCAL_PATH)/../external/exoplayer/Android.mk)
$(info "have exoplayer source code")
else
$(info "no exoplayer source code, add prebuilts")
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := exoplayer
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_FRAMEWORK)
LOCAL_SRC_FILES := exoplayer.jar
#LOCAL_DEX_PREOPT := false

include $(BUILD_PREBUILT)

endif

