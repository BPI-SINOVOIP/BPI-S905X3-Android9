LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  RemoteControl.cpp \
  RemoteControlImpl.cpp \
  RemoteControlServer.cpp

LOCAL_C_INCLUDES += \
    external/libcxx/include \
    system/libhidl/transport/include/hidl

LOCAL_SHARED_LIBRARIES := \
  vendor.amlogic.hardware.remotecontrol@1.0 \
  libhidlbase \
  libhidltransport \
  libutils \
  libcutils \
  liblog

LOCAL_MODULE:= libremotecontrolserver

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)


LOCAL_SRC_FILES := server_main.cpp

LOCAL_C_INCLUDES := \
   external/libcxx/include \
   hardware/libhardware/include \
   system/core/base/include \
   system/libhidl/transport \
   frameworks/native/libs/binder/include \
   hardware/libhardware/include \
   system/libhidl/transport/include/hidl \
   system/core/libutils/include \
   system/core/liblog/include

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  liblog \
  libbinder \
  libremotecontrolserver

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := rc_server

LOCAL_CPPFLAGS += -std=c++14
LOCAL_INIT_RC := rc_server.rc

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)



