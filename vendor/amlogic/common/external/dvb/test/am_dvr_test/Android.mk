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
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES:= am_dvr_test.c
LOCAL_MODULE:= am_dvr_test
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include/am_adp $(LOCAL_PATH)/../../android/ndk/include \
			$(AMADEC_C_INCLUDES)\
		    vendor/amlogic/frameworks/av/LibPlayer/amcodec/include\
                    vendor/amlogic/frameworks/av/LibPlayer/dvbplayer/include\
                    vendor/amlogic/frameworks/av/LibPlayer/amadec/include\
                    common/include/linux/amlogic
LOCAL_SHARED_LIBRARIES := libam_adp $(AMADEC_LIBS) libcutils liblog libc libdl
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES:= am_dvr_keyladder_test.c
LOCAL_MODULE:= am_dvr_test_keyladder
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS+=-DANDROID -DAMLINUX -DCHIP_8226M -DLINUX_DVB_FEND
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include/am_adp $(LOCAL_PATH)/../../android/ndk/include \
			$(AMADEC_C_INCLUDES)\
		    vendor/amlogic/frameworks/av/LibPlayer/amcodec/include\
                    vendor/amlogic/frameworks/av/LibPlayer/dvbplayer/include\
                    vendor/amlogic/frameworks/av/LibPlayer/amadec/include\
                    common/include/linux/amlogic
LOCAL_SHARED_LIBRARIES := libam_adp $(AMADEC_LIBS) libcutils liblog libc libdl
include $(BUILD_EXECUTABLE)
