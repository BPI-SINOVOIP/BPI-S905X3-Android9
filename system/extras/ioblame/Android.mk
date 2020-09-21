LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PREBUILT_EXECUTABLES := ioblame.sh
include $(BUILD_HOST_PREBUILT)
