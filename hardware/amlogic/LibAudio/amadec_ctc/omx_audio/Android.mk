LOCAL_PATH:= $(call my-dir)


###################module make file for libamadec_omx_api ######################################
include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -fPIC -D_POSIX_SOURCE  -DDOLBY_DDPDEC51_MULTICHANNEL_ENDPOINT
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/include \
    frameworks/native/include/media/openmax \
    frameworks/av/include/media/stagefright \
    frameworks/native/include/utils \
    frameworks/av/media/libmediaextractor/include/media \
    hardware/amlogic/media/ammediaext \
    hardware/amlogic/LibAudio/amadec  \
    hardware/amlogic/LibAudio/amadec/include \
    hardware/amlogic/media/amavutils/include \
    hardware/amlogic//audio/utils/include/

LOCAL_SRC_FILES := \
           adec_omx.cpp \
           audio_mediasource.cpp \
           DDP_mediasource.cpp \
           ALAC_mediasource.cpp \
           MP3_mediasource.cpp \
           ASF_mediasource.cpp  \
           DTSHD_mediasource.cpp \
           Vorbis_mediasource.cpp \
           THD_mediasource.cpp

LOCAL_MODULE := libamadec_omx_api
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc libstagefright \
                          libmediaextractor libstagefright_omx  liblog libamavutils libstagefright_foundation  libaudioclient
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
#########################################################

