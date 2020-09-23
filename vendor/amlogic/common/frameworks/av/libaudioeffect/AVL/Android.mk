LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libavl

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libutils \
    libamaudioutils

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    hardware/amlogic/audio/utils/ini/include \
    hardware/libhardware/include/hardware \
    hardware/libhardware/include \
    system/media/audio/include

LOCAL_SRC_FILES += Avl.cpp

LOCAL_LDFLAGS_arm  += $(LOCAL_PATH)/lib_aml_agc.a
LOCAL_LDFLAGS_arm64  += $(LOCAL_PATH)/lib_aml_agc64.a
LOCAL_MULTILIB := both
LOCAL_PRELINK_MODULE := false

LOCAL_LDLIBS   +=  -llog
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE_RELATIVE_PATH := soundfx

include $(BUILD_SHARED_LIBRARY)
