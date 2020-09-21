LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES:= am_fend_test.c

LOCAL_MODULE:= am_fend_test

LOCAL_MODULE_TAGS := optional

#LOCAL_MULTILIB := 32

LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include/am_adp $(LOCAL_PATH)/../../android/ndk/include
				
LOCAL_SHARED_LIBRARIES := libam_adp liblog libc

include $(BUILD_EXECUTABLE)

