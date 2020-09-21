LOCAL_PATH:= $(call my-dir)

ifneq (0, $(shell expr $(PLATFORM_VERSION) \< 9))
include $(CLEAR_VARS)

LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := \
	AmMediaDefsExt.cpp	\

LOCAL_C_INCLUDES += \
	$(TOP)/frameworks/native/include/media/openmax  \
	$(TOP)/system/core/include/utils  \
	$(LOCAL_PATH)/include/

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        libdl \
        libutils \
        liblog \


LOCAL_MODULE:= libammediaext
include $(BUILD_SHARED_LIBRARY)
endif
