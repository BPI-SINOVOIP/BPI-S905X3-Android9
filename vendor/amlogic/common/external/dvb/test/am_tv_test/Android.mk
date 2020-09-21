LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES:= am_tv_test.c

LOCAL_MODULE:= am_tv_test

LOCAL_MODULE_TAGS := optional

#LOCAL_MULTILIB := 32

LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include/am_adp \
					$(LOCAL_PATH)/../../android/ndk/include \
					$(LOCAL_PATH)/../../include/am_mw \
					$(LOCAL_PATH)/../../include/am_app \
					packages/amlogic/LibPlayer/amadec/include\
					packages/amlogic/LibPlayer/amcodec/include\
					packages/amlogic/LibPlayer/amffmpeg\
					packages/amlogic/LibPlayer/amplayer\
					external/sqlite/dist\
					common/include/linux/amlogic
LOCAL_STATIC_LIBRARIES := libam_app libam_mw libam_adp
LOCAL_SHARED_LIBRARIES := libicuuc libzvbi libsqlite libamplayer liblog libc libcutils

include $(BUILD_EXECUTABLE)

