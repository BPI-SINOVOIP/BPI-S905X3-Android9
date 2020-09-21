LOCAL_PATH:= $(call my-dir)
# build for subtitle server
# =========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  SubtitleServerHal.cpp \
  main_subtitleserver.cpp \


LOCAL_SHARED_LIBRARIES := \
  libutils                    \
  vendor.amlogic.hardware.subtitleserver@1.0 \
  libhidlbase \
  libhidltransport \
  libbinder                   \
  libcutils                   \
  liblog                      \
  libjpeg                     \
  libz			      \
  libdl                       \
  libcurl


LOCAL_C_INCLUDES += \
  frameworks/native/include/utils \
  frameworks/base/include/utils         \
  system/libhidl/transport/include/hidl \
  system/core/libutils/include \
  system/core/liblog/include \
  system/core/libsystem/include \
  system/core/libcutils/include \
  external/libcxx/include \
  frameworks/native/libs/binder/include \
  system/core/base/include \
  system/libhidl/transport/base/1.0

LOCAL_MODULE_TAGS := optional

LOCAL_CPPFLAGS += -std=c++14

LOCAL_MODULE:= subtitleserver
#LOCAL_32_BIT_ONLY := true
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_INIT_RC := subtitleserver.rc
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
