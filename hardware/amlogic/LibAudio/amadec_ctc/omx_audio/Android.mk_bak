LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -fPIC -D_POSIX_SOURCE  -DDOLBY_DDPDEC51_MULTICHANNEL_ENDPOINT

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/../     \
    $(LOCAL_PATH)/../include \
    frameworks/av/include \
    frameworks/native/include/media/openmax \
    frameworks/native/libs/nativewindow/include/system \
    frameworks/av/include/media/stagefright \
    frameworks/native/include/utils

LOCAL_SRC_FILES := \
    adec_omx.cpp audio_mediasource.cpp

LOCAL_MODULE := libamadec_omx_api

LOCAL_ARM_MODE := arm

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc libstagefright \
                          libstagefright_omx libmedia_native liblog \
                          libstagefright_foundation \
                          libmediaextractor

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
