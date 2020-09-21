LOCAL_PATH := $(call my-dir)

### shared library

include $(CLEAR_VARS)

LIB_TV_BINDER_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)/tv/frameworks/libtvbinder)

LOCAL_SRC_FILES := \
    com_droidlogic_app_tv_TvControlManager.cpp

LOCAL_SHARED_LIBRARIES :=  \
    vendor.amlogic.hardware.tvserver@1.0 \
    vendor.amlogic.hardware.hdmicec@1.0 \
    libhidlbase \
    libcutils \
    libutils \
    libtvbinder \
    liblog

LOCAL_C_INCLUDES += $(JNI_H_INCLUDE) \
                    frameworks/base/core/jni/include \
                    libnativehelper/include/nativehelpers \
                    hardware/libhardware/include \
                    $(LIB_TV_BINDER_PATH)/include \

LOCAL_MODULE := libtv_jni
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true
LOCAL_PRIVATE_PLATFORM_APIS := true
include $(BUILD_SHARED_LIBRARY)
