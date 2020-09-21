LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    InstabootSupport.cpp

LOCAL_C_INCLUDES += system/core/fs_mgr/include \
        external/fw_env \
        system/extras/ext4_utils \
        system/vold \
        vendor/amlogic/frameworks/services/systemcontrol \
        vendor/amlogic/frameworks/tv/include \
        $(BOARD_AML_VENDOR_PATH)/frameworks/services/systemcontrol \
        $(BOARD_AML_VENDOR_PATH)/frameworks/tv/include

LOCAL_MODULE := libinstaboot_support

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        libdl \
        libutils \
        liblog \
        libbinder \
        libsystemcontrolservice \
        libc \
        libtvplay

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
