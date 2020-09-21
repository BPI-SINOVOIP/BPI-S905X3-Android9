LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  KeyBurn.cpp

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  liblog \
  libbinder \
  libsystemcontrolservice

LOCAL_SHARED_LIBRARIES += \
  vendor.amlogic.hardware.systemcontrol@1.1 \
  libbase \
  libhidlbase \
  libhidltransport

LOCAL_C_INCLUDES += \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol/PQ/include \
  $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol

LOCAL_MODULE:= libkeys_burn

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)

