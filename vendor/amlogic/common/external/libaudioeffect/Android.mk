#LOCAL_PATH := $(call my-dir)

#--------------EQ by amlogic------------------------#
#include $(CLEAR_VARS)
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE := libhpeq.so
#
#LOCAL_SRC_FILES := EQ/$(LOCAL_MODULE)
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/soundfx
#include $(BUILD_PREBUILT)

#-------------SRS by DTS-----------------------------#
#include $(CLEAR_VARS)
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE := libsrs.so
#
#LOCAL_SRC_FILES := SRS/$(LOCAL_MODULE)
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/soundfx
#include $(BUILD_PREBUILT)

