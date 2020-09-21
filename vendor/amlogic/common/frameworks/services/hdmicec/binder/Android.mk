LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
    HdmiCecHidlClient.cpp \
    HdmiCecBase.cpp

LOCAL_C_INCLUDES += \
   external/libcxx/include \
   hardware/libhardware/include \
   frameworks/native/libs/binder/include

LOCAL_CPPFLAGS += -std=c++14

LOCAL_SHARED_LIBRARIES := \
    vendor.amlogic.hardware.hdmicec@1.0 \
    libbase \
    libhidlbase \
    libhidltransport \
    libcutils \
    libutils \
    liblog


LOCAL_MODULE:= libhdmicec

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)
