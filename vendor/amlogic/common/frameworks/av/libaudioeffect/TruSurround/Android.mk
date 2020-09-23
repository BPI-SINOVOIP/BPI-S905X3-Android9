LOCAL_PATH := $(call my-dir)
# SRS library
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_ARM_MODE := arm

LOCAL_MODULE := libsrswrapper

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libutils \
    libamaudioutils

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/srs_wrapper \
    hardware/amlogic/audio/utils/ini/include \
    hardware/libhardware/include/hardware \
    hardware/libhardware/include \
    system/media/audio/include

LOCAL_SRC_FILES := tshd_wrapper.cpp

LOCAL_CFLAGS += -O2

LOCAL_LDLIBS   +=  -llog
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE_RELATIVE_PATH := soundfx

include $(BUILD_SHARED_LIBRARY)
