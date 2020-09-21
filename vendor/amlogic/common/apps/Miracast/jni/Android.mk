LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    com_droidlogic_miracast_wfd.cpp

LOCAL_C_INCLUDES := \
    $(JNI_H_INCLUDE) \
    libnativehelper/include_jni \
    system/core/libutils/include \
    system/core/liblog/include  \
    frameworks/base/core/jni/include/

LOCAL_SHARED_LIBRARIES:= \
        libandroid_runtime\
        libnativehelper\
        libbinder \
        libgui \
        libmedia \
        libstagefright \
        libstagefright_foundation \
        libstagefright_wfd_sink \
        liblog \
        libutils \
        libcutils

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
LOCAL_C_INCLUDES+=$(TOP)/vendor/amlogic/common/frameworks/av/libstagefright/wifi-display
LOCAL_PRODUCT_MODULE := true
LOCAL_SHARED_LIBRARIES += \
        vendor.amlogic.hardware.miracast_hdcp2@1.0
else
LOCAL_C_INCLUDES+=$(TOP)/vendor/amlogic/frameworks/av/libstagefright/wifi-display
endif

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_MODULE := libwfd_jni
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
