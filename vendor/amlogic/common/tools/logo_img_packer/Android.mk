# Copyright 2005 The Android Open Source Project
#
#

LOCAL_PATH:= $(call my-dir)

# target tool
# =========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := res_pack.cpp crc32.cpp
LOCAL_MODULE := logo_img_packer

ifeq ($(HOST_OS),cygwin)
LOCAL_CFLAGS += -DWIN32_EXE
endif
ifeq ($(HOST_OS),darwin)
LOCAL_CFLAGS += -DMACOSX_RSRC
endif
ifeq ($(HOST_OS),linux)
endif

LOCAL_STATIC_LIBRARIES := libhost
LOCAL_C_INCLUDES := build/libs/host/include
#LOCAL_ACP_UNAVAILABLE := true
include $(BUILD_HOST_EXECUTABLE)

# host tool
# =========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := res_pack.cpp crc32.cpp
LOCAL_MODULE := logo_img_packer
LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_EXECUTABLE)

