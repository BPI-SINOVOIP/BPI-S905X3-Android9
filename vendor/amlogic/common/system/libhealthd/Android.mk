# Copyright 2013 The Android Open Source Project

ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := healthd_board_mbox_default.cpp
LOCAL_MODULE := libhealthd.mboxdefault
LOCAL_C_INCLUDES := system/core/healthd \
                    system/core/healthd/include/healthd
LOCAL_CFLAGS := -Werror

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := healthd_board_tablet_default.cpp
LOCAL_MODULE := libhealthd.tablet
LOCAL_C_INCLUDES := system/core/healthd \
                    system/core/healthd/include/healthd
LOCAL_CFLAGS := -Werror

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_STATIC_LIBRARY)

endif
