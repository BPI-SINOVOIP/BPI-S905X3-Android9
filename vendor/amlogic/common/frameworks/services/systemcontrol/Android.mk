LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  SystemControlClient.cpp \
  ISystemControlService.cpp \
  ISystemControlNotify.cpp

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  liblog \
  libbinder

LOCAL_SHARED_LIBRARIES += \
  vendor.amlogic.hardware.systemcontrol@1.0 \
  vendor.amlogic.hardware.systemcontrol@1.1 \
  libbase \
  libhidlbase \
  libhidltransport

LOCAL_C_INCLUDES += \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol/PQ/include

LOCAL_CFLAGS += -Wno-unused-variable -Wno-unused-parameter

LOCAL_MODULE:= libsystemcontrolservice

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM), meson8)
LOCAL_CFLAGS += -DMESON8_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxbaby)
LOCAL_CFLAGS += -DGXBABY_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxtvbb)
LOCAL_CFLAGS += -DGXTVBB_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxl)
LOCAL_CFLAGS += -DGXL_ENVSIZE
endif

PQ_INCLUDE_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol/PQ/include)
LIB_SQLITE_PATH := $(wildcard external/sqlite/dist)

LOCAL_CFLAGS += -DHDCP_AUTHENTICATION
LOCAL_CPPFLAGS += -std=c++14
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_CFLAGS += -Wno-unused-variable -Wno-unused-parameter

ifeq ($(HWC_DYNAMIC_SWITCH_VIU), true)
LOCAL_CFLAGS += -DHWC_DYNAMIC_SWITCH_VIU
endif

LOCAL_SRC_FILES:= \
  main_systemcontrol.cpp \
  ubootenv/Ubootenv.cpp \
  VdcLoop.c \
  SysWrite.cpp \
  SystemControl.cpp \
  SystemControlHal.cpp \
  SystemControlService.cpp \
  DisplayMode.cpp \
  Dimension.cpp \
  SysTokenizer.cpp \
  UEventObserver.cpp \
  HDCP/HdcpKeyDecrypt.cpp \
  HDCP/HDCPRxKey.cpp \
  HDCP/HDCPRxAuth.cpp \
  HDCP/HDCPTxAuth.cpp \
  HDCP/sha1.cpp \
  HDCP/HDCPRx22ImgKey.cpp \
  FrameRateAutoAdaption.cpp \
  FormatColorDepth.cpp \
  keymaster_hidl_hal_test.cpp \
  authorization_set.cpp \
  attestation_record.cpp \
  key_param_output.cpp \
  keystore_tags_utils.cpp \
  VtsHalHidlTargetTestBase.cpp \
  VtsHalHidlTargetTestEnvBase.cpp

  #HDCP/aes.cpp

LOCAL_SHARED_LIBRARIES := \
  libsystemcontrolservice \
  libcutils \
  libutils \
  liblog \
  libbinder \
  libm \
  libpqcontrol \
  libsqlite \
  libhwbinder \
  libfbc

LOCAL_SHARED_LIBRARIES += \
  vendor.amlogic.hardware.droidvold@1.0 \
  vendor.amlogic.hardware.systemcontrol@1.0 \
  vendor.amlogic.hardware.systemcontrol@1.1 \
  libbase \
  libhidlbase \
  libhidltransport

LOCAL_C_INCLUDES := \
  external/zlib \
  external/libcxx/include \
  system/core/include \
  system/libhidl/transport/include/hidl \
  external/googletest/googletest/include \
  hardware/libhardware/include \
  $(PQ_INCLUDE_PATH) \
  $(LIB_SQLITE_PATH) \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/fbc_tool/libfbc/include

LOCAL_MODULE:= systemcontrol

LOCAL_INIT_RC := systemcontrol.rc

LOCAL_STATIC_LIBRARIES := \
  libz \
  android.hardware.keymaster@3.0 \
  libcrypto \
  libsoftkeymasterdevice \
  libgtest \


ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_EXECUTABLE)


# build for recovery mode
# =========================================================
include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM), meson8)
LOCAL_CFLAGS += -DMESON8_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxbaby)
LOCAL_CFLAGS += -DGXBABY_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxtvbb)
LOCAL_CFLAGS += -DGXTVBB_ENVSIZE
endif

LOCAL_CFLAGS += -DRECOVERY_MODE
LOCAL_CPPFLAGS += -std=c++14
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_CFLAGS += -Wno-unused-variable -Wno-unused-parameter

LOCAL_SRC_FILES:= \
  main_recovery.cpp \
  ubootenv/Ubootenv.cpp \
  SysWrite.cpp \
  DisplayMode.cpp \
  SysTokenizer.cpp \
  UEventObserver.cpp \
  HDCP/HdcpKeyDecrypt.cpp \
  HDCP/HDCPRxKey.cpp \
  HDCP/HDCPRxAuth.cpp \
  HDCP/HDCPTxAuth.cpp \
  FrameRateAutoAdaption.cpp \
  FormatColorDepth.cpp

LOCAL_STATIC_LIBRARIES := \
  libz \
  libcutils

LOCAL_C_INCLUDES := \
  external/zlib \
  external/libcxx/include \
  system/core/libcutils/include \
  system/core/libutils/include \
  system/core/liblog/include

#LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/utilities
LOCAL_MODULE:= systemcontrol_static

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
