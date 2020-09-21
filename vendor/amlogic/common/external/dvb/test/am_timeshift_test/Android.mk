LOCAL_PATH := $(call my-dir)


AMLOGIC_LIBPLAYER :=y

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 25)))
AMADEC_C_INCLUDES:=hardware/amlogic/media/amcodec/include\
       hardware/amlogic/LibAudio/amadec/include
AMADEC_LIBS:=libamadec
else
AMADEC_C_INCLUDES:=vendor/amlogic/frameworks/av/LibPlayer/amcodec/include\
       vendor/amlogic/frameworks/av/LibPlayer/amadec/include
AMADEC_LIBS:=libamplayer
endif
ifeq ($(AMLOGIC_LIBPLAYER), y)
LOCAL_CFLAGS+=-DAMLOGIC_LIBPLAYER
endif

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= am_timeshift_test.c
LOCAL_MODULE:= am_timeshift_test
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true
LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include/am_adp\
            $(LOCAL_PATH)/../../include/am_mw\
            $(LOCAL_PATH)/../../android/ndk/include \
            $(AMADEC_C_INCLUDES)\
            external/sqlite/dist
LOCAL_SHARED_LIBRARIES := libam_mw libam_adp
LOCAL_SHARED_LIBRARIES += libcutils liblog libc
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= am_timeshift_new_test.c
LOCAL_MODULE:= am_timeshift_new_test
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true
LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include/am_adp\
            $(LOCAL_PATH)/../../include/am_mw\
            $(LOCAL_PATH)/../../android/ndk/include \
            $(AMADEC_C_INCLUDES)\
            external/sqlite/dist
LOCAL_SHARED_LIBRARIES := libam_mw libam_adp
LOCAL_SHARED_LIBRARIES += libcutils liblog libc
LOCAL_REQUIRED_MODULES := libam_mw libam_adp
include $(BUILD_EXECUTABLE)

