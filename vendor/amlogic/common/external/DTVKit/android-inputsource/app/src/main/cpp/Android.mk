LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

DTVKITSOURCE_PLATFORM := amlogic

LOCAL_MODULE := libplatform
#LOCAL_SDK_VERSION := 21 # for normal app (ndk jni)
LOCAL_SHARED_LIBRARIES := liblog libandroid libnativehelper

$(info inputsource: platform '$(DTVKITSOURCE_PLATFORM)')
ifeq (broadcom,$(DTVKITSOURCE_PLATFORM))
LOCAL_C_INCLUDES += system/core/libcutils/include
LOCAL_C_INCLUDES += vendor/broadcom/bcm_platform/media/libbcmsideband/include
LOCAL_C_INCLUDES += vendor/broadcom/bcm_platform/media/libbcmsidebandplayer/include
LOCAL_SHARED_LIBRARIES += libbcmsideband
LOCAL_CFLAGS += -DPLATFORM_BROADCOM
endif
ifeq (amlogic,$(DTVKITSOURCE_PLATFORM))
LOCAL_CFLAGS += -DPLATFORM_AMLOGIC
LOCAL_C_INCLUDES += $(JNI_H_INCLUDE) hardware/amlogic/gralloc\
    frameworks/base/include \
    frameworks/native/include \
    libnativehelper/include/nativehelper

LOCAL_SHARED_LIBRARIES += \
    libcutils \
    libutils \
    libgui \
    libnativehelper \
    libandroid_runtime \
    libui
endif



LOCAL_SRC_FILES := platform.cpp

include $(BUILD_SHARED_LIBRARY)

