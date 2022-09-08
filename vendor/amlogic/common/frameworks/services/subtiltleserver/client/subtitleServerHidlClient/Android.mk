LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
    SubtitleServerHidlClient.cpp

LOCAL_SHARED_LIBRARIES := \
    vendor.amlogic.hardware.subtitleserver@1.0 \
    libbase \
    libhidlbase \
    libhidltransport \
    liblog \
    libcutils \
    libutilscallstack \
    libutils

LOCAL_C_INCLUDES += \
  system/libhidl/transport/include/hidl \
  system/libhidl/libhidlmemory/include

LOCAL_C_INCLUDES += \
   external/libcxx/include

LOCAL_CFLAGS += -std=c++14 -Wno-unused-parameter -Wno-unused-variable

LOCAL_MODULE:= libsubtitlebinder

#ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
#endif

#include $(BUILD_SHARED_LIBRARY)
