LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := ResourceManagerService.cpp ServiceLog.cpp

LOCAL_SHARED_LIBRARIES := libmedia libmediautils libbinder libutils liblog

LOCAL_MODULE:= libresourcemanagerservice

LOCAL_32_BIT_ONLY := true

LOCAL_C_INCLUDES += \
    frameworks/av/include

LOCAL_CFLAGS += -Werror -Wall

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
