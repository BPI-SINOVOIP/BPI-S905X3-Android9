LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  ISubTitleService.cpp

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  libbinder

LOCAL_MODULE:= libsubtitleservice

LOCAL_MODULE_TAGS := optional

#include $(BUILD_SHARED_LIBRARY)