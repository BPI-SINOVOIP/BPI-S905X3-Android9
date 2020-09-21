LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    BitFieldParser.cpp \
    FrameScanner.cpp \
    AC3FrameScanner.cpp \
    DTSFrameScanner.cpp \
    SPDIFEncoder.cpp

LOCAL_C_INCLUDES := \
    system/core/libutils/include \
    system/media/audio/include

LOCAL_MODULE := libdroidaudiospdif

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
