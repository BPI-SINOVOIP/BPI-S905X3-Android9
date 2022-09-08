LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= ApiTest.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../

LOCAL_SHARED_LIBRARIES :=       \
  liblog    \
  libutils  \
  libcutils \
  libSubtitleClient

LOCAL_C_FLAGS := -Wno-unused-variable   \
                 -Wno-unused-parameter

LOCAL_MODULE := subtitle_api_test
LOCAL_MODULE_TAGS := optional

LOCAL_SANITIZE := address

include $(BUILD_EXECUTABLE)
