LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES:= am_upd_test.c

LOCAL_MODULE:= am_upd_test

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include/am_adp \
					$(LOCAL_PATH)/../../android/ndk/include \
					$(LOCAL_PATH)/../../include/am_mw \
				    $(LOCAL_PATH)/../../include/am_mw/libdvbsi\
				    $(LOCAL_PATH)/../../include/am_mw/libdvbsi/descriptors\
				    $(LOCAL_PATH)/../../include/am_mw/libdvbsi/tables\
				    $(LOCAL_PATH)/../../include/am_mw/atsc
LOCAL_SHARED_LIBRARIES := libam_mw libam_adp liblog libc

LOCAL_LDFLAGS += -ldl

include $(BUILD_EXECUTABLE)
