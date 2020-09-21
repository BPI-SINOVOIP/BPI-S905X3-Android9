
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../amavutils/include \
	$(JNI_H_INCLUDE)

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libmedia \
	libz \
	libbinder \
	libcutils \
	libc \
	libamavutils \
	liblog

#LOCAL_SHARED_LIBRARIES += libandroid_runtime libnativehelper

LOCAL_MODULE := libamvdec
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../amavutils/include \
	$(JNI_H_INCLUDE)


LOCAL_SHARED_LIBRARIES := \
	libutils \
	libmedia \
	libz \
	libbinder \
	libcutils \
	libc \
	libamavutils \
	liblog

#LOCAL_SHARED_LIBRARIES += libandroid_runtime libnativehelper

LOCAL_MODULE := libamvdec
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)
