################################################

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
    MessageQueue.cpp \
    Semaphore.cpp \
    ErrorUtils.cpp 

    
ifeq ($(BOARD_USE_USB_CAMERA),true)
    LOCAL_SRC_FILES += util.cpp
endif

LOCAL_SHARED_LIBRARIES:= \
    libdl \
    libui \
    libbinder \
    libutils \
    libcutils

LOCAL_C_INCLUDES += \
	frameworks/base/include/utils \
	bionic/libc/include
	
LOCAL_CFLAGS += -fno-short-enums 

# LOCAL_CFLAGS +=

LOCAL_MODULE:= libtiutils
LOCAL_MODULE_TAGS:= optional

include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)
