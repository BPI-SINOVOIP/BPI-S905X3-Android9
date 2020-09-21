LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
    TvServerHidlClient.cpp

LOCAL_SHARED_LIBRARIES := \
    vendor.amlogic.hardware.tvserver@1.0 \
    libbase \
    libhidlbase \
    libhidltransport \
    liblog \
    libcutils \
    libutils

LOCAL_C_INCLUDES += \
  system/libhidl/transport/include/hidl \
  system/libhidl/libhidlmemory/include

LOCAL_C_INCLUDES += \
   external/libcxx/include

LOCAL_CPPFLAGS += -std=c++14

LOCAL_MODULE:= libtvbinder

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
