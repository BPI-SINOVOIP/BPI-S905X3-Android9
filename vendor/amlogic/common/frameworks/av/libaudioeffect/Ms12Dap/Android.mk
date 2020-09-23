LOCAL_PATH := $(call my-dir)
# MS12 DAP audio effect library
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_ARM_MODE := arm

LOCAL_MODULE := libms12dapwrapper

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libutils \
    libamaudioutils

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    hardware/amlogic/audio/utils/ini/include \
    hardware/libhardware/include \
    system/media/audio/include \
    vendor/amlogic/frameworks/av/libaudioeffect/Utility

LOCAL_SRC_FILES := ms12_dap_wapper.cpp
LOCAL_SRC_FILES += ../Utility/AudioFade.c

LOCAL_CFLAGS += -O2

LOCAL_LDLIBS   +=  -llog
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE_RELATIVE_PATH := soundfx

include $(BUILD_SHARED_LIBRARY)
