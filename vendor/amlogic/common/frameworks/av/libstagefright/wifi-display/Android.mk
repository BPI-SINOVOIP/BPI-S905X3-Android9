LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(BOARD_AML_MEDIA_HAL_CONFIG)
LOCAL_SRC_FILES:= \
        sink/LinearRegression.cpp       \
        sink/RTPSink.cpp                \
        sink/TunnelRenderer.cpp         \
        sink/WifiDisplaySink.cpp        \
        sink/Utils.cpp                  \
        sink/AmANetworkSession.cpp

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/av/media/libstagefright \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/av/media/libstagefright/mpeg2ts \
        $(TOP)/frameworks/native/include/media/hardware \
        $(TOP)/frameworks/native/headers/media_plugin

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_C_INCLUDES += \
        $(TOP)/vendor/amlogic/common/frameworks/av/mediaextconfig/include
else
LOCAL_C_INCLUDES += \
        $(TOP)/vendor/amlogic/frameworks/av/mediaextconfig/include
endif

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_SHARED_LIBRARIES:= \
        libbinder                       \
        libcutils                       \
        liblog                          \
        libgui                          \
        libmedia                        \
        libstagefright                  \
        libstagefright_foundation       \
        libui                           \
        libutils

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_SHARED_LIBRARIES += \
        vendor.amlogic.hardware.miracast_hdcp2@1.0 \
        libhardware         \
        libhidlbase         \
        libhidltransport
endif

LOCAL_MODULE:= libstagefright_wfd_sink

LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
