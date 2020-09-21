LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BUILD_SH_TELECOM_APKS),true)
LOCAL_CFLAGS += -DSH_TELCOM_SUPPORT
endif

LOCAL_CFLAGS += -D__STDC_CONSTANT_MACROS -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_ARM_MODE := arm
LOCAL_MODULE    := libFFExtractor
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
	amffplayer.cpp \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \= 19))

LOCAL_C_INCLUDES += $(TOP)/external/ffmpeg \

else ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 23))

LOCAL_C_INCLUDES += \
	$(TOP)/vendor/amlogic/common/external/ffmpeg \
#	$(TOP)/vendor/amlogic/common/iptvmiddlewave/AmIptvMedia/contrib/ffmpeg40 \

endif

LOCAL_SHARED_LIBRARIES += liblog libcutils libutils

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \= 19))

LOCAL_SHARED_LIBRARIES += libamffmpeg

else ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 23))

LOCAL_SHARED_LIBRARIES += libffmpeg30

endif

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_EXECUTABLE)
