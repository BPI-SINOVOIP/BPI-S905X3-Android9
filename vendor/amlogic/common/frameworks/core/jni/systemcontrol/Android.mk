LOCAL_PATH := $(call my-dir)

### shared library

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    com_droidlogic_app_SystemControlManager.cpp

$(warning $(JNI_H_INCLUDE))
LOCAL_C_INCLUDES += $(JNI_H_INCLUDE) \
                    frameworks/base/core/jni/include \
                    $(LOCAL_PATH)/../../../services/systemcontrol/PQ/include \
                    $(LOCAL_PATH)/../../../services/systemcontrol \
                    libnativehelper/include/nativehelpers

LOCAL_MODULE := libsystemcontrol_jni

LOCAL_SHARED_LIBRARIES :=  \
    vendor.amlogic.hardware.systemcontrol@1.0 \
    vendor.amlogic.hardware.systemcontrol@1.1 \
    libsystemcontrolclient \
    libcutils \
    libutils \
    libhidlbase \
    liblog


LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true
LOCAL_PRIVATE_PLATFORM_APIS := true
include $(BUILD_SHARED_LIBRARY)
