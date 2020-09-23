LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE    := AudioEffectTool

LOCAL_SRC_FILES := main.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libaudioclient \
    libmedia \
    libmedia_helper \
    libmediaplayerservice \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../VirtualX \

include $(BUILD_EXECUTABLE)

#for tuning tool
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE    := TuningTool

LOCAL_SRC_FILES := main2.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libaudioclient \
    libmedia \
    libmedia_helper \
    libmediaplayerservice \

include $(BUILD_EXECUTABLE)