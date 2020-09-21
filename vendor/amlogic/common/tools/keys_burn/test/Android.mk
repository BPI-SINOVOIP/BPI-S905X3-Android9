# build for test
# =========================================================
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  KeyBurnTest.cpp

LOCAL_SHARED_LIBRARIES := \
  libcutils \
  libutils  \
  liblog \
  libkeys_burn

LOCAL_MODULE:= keys_burn_test

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)
