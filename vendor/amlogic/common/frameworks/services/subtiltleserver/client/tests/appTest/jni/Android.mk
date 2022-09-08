LOCAL_PATH:= $(call my-dir)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= ApiTestJni.cpp

LOCAL_C_INCLUDES :=     \
      vendor/amlogic/common/frameworks/services/subtiltleserver/client \
      $(JNI_H_INCLUDE)

LOCAL_SHARED_LIBRARIES :=       \
      liblog    \
      libSubtitleClient

LOCAL_C_FLAGS += -Wno-unused-variable

LOCAL_MODULE := libsubtitleApiTestJni
LOCAL_MODULE_TAGS := optional

LOCAL_CPPFLAGS += -std=c++14 -Wno-unused-parameter -Wno-unused-const-variable -O0
include $(BUILD_SHARED_LIBRARY)
