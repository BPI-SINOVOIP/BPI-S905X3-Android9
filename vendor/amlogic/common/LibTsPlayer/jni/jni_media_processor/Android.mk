LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE    := libCTC_MediaProcessorjni

LOCAL_SRC_FILES := MediaProcessorJni.cpp \
			Proxy_MediaProcessor.cpp

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \= 19))

ifneq (,$(wildcard vendor/amlogic/frameworks/av/LibPlayer))
LIBPLAYER_PATH:=$(TOP)/vendor/amlogic/frameworks/av/LibPlayer
else
LIBPLAYER_PATH := $(TOP)/packages/amlogic/LibPlayer
endif

else ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 23))

HARDWARE_PATH := $(TOP)/hardware/amlogic

endif

LOCAL_C_INCLUDES += \
	$(JNI_H_INCLUDE) \
	$(LOCAL_PATH)/../include \
	$(TOP)/frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/include/media/openmax \
	$(TOP)/frameworks/av/media/libstagefright/mpeg2ts \

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \= 19))

LOCAL_C_INCLUDES += \
	$(ANDDROID_PLATFORM)/frameworks/native/include \
	$(LIBPLAYER_PATH)/amplayer/player/include \
	$(LIBPLAYER_PATH)/amplayer/control/include \
	$(LIBPLAYER_PATH)/amffmpeg \
	$(LIBPLAYER_PATH)/amcodec/include \
	$(LIBPLAYER_PATH)/amadec/include \
	$(LIBPLAYER_PATH)/amavutils/include \

else ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 23))

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../include/android9 \
	$(TOP)/frameworks/native/include \
	$(HARDWARE_PATH)/media/amcodec/include \
	$(HARDWARE_PATH)/LibAudio/amadec/include \
	$(HARDWARE_PATH)/media/amavutils/include \
	$(TOP)/vendor/amlogic/common/external/ffmpeg \
#	$(TOP)/vendor/amlogic/common/iptvmiddlewave/AmIptvMedia/contrib/ffmpeg40 \

endif


LOCAL_SHARED_LIBRARIES += 	\
			libandroid_runtime \
			libnativehelper \
			libcutils \
			libutils \
			libbinder \
			libmedia\
			libgui \
			liblog \
			libCTC_MediaProcessor \

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \= 19))

LOCAL_SHARED_LIBRARIES += \
		libamplayer	\

else ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 23))

LOCAL_SHARED_LIBRARIES += \
		libffmpeg30	\
		libstagefright

endif

include $(BUILD_SHARED_LIBRARY)
