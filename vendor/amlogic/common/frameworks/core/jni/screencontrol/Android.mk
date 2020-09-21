LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    com_droidlogic_app_ScreenControlManager.cpp

$(warning $(JNI_H_INCLUDE))
LOCAL_C_INCLUDES += \
	$(JNI_H_INCLUDE) \
    frameworks/base/core/jni/include \
    $(BOARD_AML_VENDOR_PATH)/frameworks/services/screencontrol \
    libnativehelper/include/nativehelpers

LOCAL_SHARED_LIBRARIES := \
	vendor.amlogic.hardware.screencontrol@1.0 \
    libscreencontrolclient \
    libcutils \
    libutils \
    liblog

LOCAL_MODULE := libscreencontrol_jni

#LOCAL_LDFLAGS += -L${TARGET_ROOT_OUT}/

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true
LOCAL_PRIVATE_PLATFORM_APIS := true

include $(BUILD_SHARED_LIBRARY)

