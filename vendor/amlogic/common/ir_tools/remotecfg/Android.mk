LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=   \
	main.c	    \
	parse.c \
	device.c   \

LOCAL_STATIC_LIBRARIES:=  \
	libcutils

LOCAL_C_INCLUDES += \
    bionic/libc/include \

LOCAL_MODULE:= remotecfg

#LOCAL_FORCE_STATIC_EXECUTABLE := true

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)