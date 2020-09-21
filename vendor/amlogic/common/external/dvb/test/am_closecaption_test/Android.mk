LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES:= am_vbi_test.c

LOCAL_MODULE:= am_vbi_test

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include/am_mw $(LOCAL_PATH)/../../android/ndk/include \
                    $(LOCAL_PATH)/../../include/am_adp \
                    $(LOCAL_PATH)/../../am_mw/am_closecaption \
                    $(LOCAL_PATH)/../../am_mw/am_closecaption/am_vbi\
                    external/sqlite/dist

LOCAL_STATIC_LIBRARIES := libam_mw
LOCAL_SHARED_LIBRARIES := libam_adp libamplayer libcutils liblog libc libsqlite

include $(BUILD_EXECUTABLE)
