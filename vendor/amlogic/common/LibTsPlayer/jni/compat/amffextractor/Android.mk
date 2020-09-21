LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \= 19))

ifeq ($(BUILD_SH_TELECOM_APKS),true)
LOCAL_CFLAGS += -DSH_TELCOM_SUPPORT
endif

endif

LOCAL_CFLAGS += -D__STDC_CONSTANT_MACROS

LOCAL_ARM_MODE := arm
LOCAL_MODULE    := libamFFExtractor
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
	amffextractor.cpp \

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \= 19))

LIBPLAYER_PATH := $(TOP)/packages/amlogic/LibPlayer
LOCAL_C_INCLUDES += \
	$(TOP)/external/ffmpeg \
	$(LIBPLAYER_PATH)/amcodec/include \
	$(LIBPLAYER_PATH)/amadec/include \
	$(LIBPLAYER_PATH)/amavutils/include \

else ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 23))

HARDWARE_PATH := $(TOP)/hardware/amlogic
LOCAL_C_INCLUDES += \
	$(HARDWARE_PATH)/media/amcodec/include \
	$(HARDWARE_PATH)/media/amavutils/include \
	$(TOP)/vendor/amlogic/common/external/ffmpeg \
#	$(TOP)/vendor/amlogic/common/iptvmiddlewave/AmIptvMedia/contrib/ffmpeg40 \

endif

LOCAL_SHARED_LIBRARIES += liblog libcutils libutils libamcodec

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \= 19))

LOCAL_SHARED_LIBRARIES += libamffmpeg libamadec

else ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 23))

LOCAL_SHARED_LIBRARIES += libffmpeg30

endif

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \= 19))

ifeq ($(TARGET_USE_OPTEEOS),true)
LOCAL_SHARED_LIBRARIES += libtelecom_iptv
LOCAL_CFLAGS += -DUSE_OPTEEOS
endif

endif

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_EXECUTABLE)
