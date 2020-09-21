LOCAL_PATH := $(call my-dir)
#use systen control service rw sysfs file

include $(CLEAR_VARS)

LOCAL_MODULE    := libam_sysfs
ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
    LOCAL_VENDOR_MODULE := false
else
    LOCAL_VENDOR_MODULE := true
endif
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := am_syswrite.cpp

LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/am_sysfs\
                    $(LOCAL_PATH)/../include/am_adp\
                    $(LOCAL_PATH)/../../../frameworks/services/systemcontrol \
                    $(LOCAL_PATH)/../../../frameworks/services/systemcontrol/PQ/include

LOCAL_SHARED_LIBRARIES+=libcutils liblog libc
#for bind

ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
    LOCAL_SHARED_LIBRARIES+=libutils  libbinder libsystemcontrolservice_system libam_adp
else
    LOCAL_SHARED_LIBRARIES+=libutils  libbinder libsystemcontrolservice libam_adp
endif

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28&& echo OK),OK)
LOCAL_SHARED_LIBRARIES+=vendor.amlogic.hardware.systemcontrol@1.0
LOCAL_SHARED_LIBRARIES+=vendor.amlogic.hardware.systemcontrol@1.1
else
LOCAL_SHARED_LIBRARIES+=vendor.amlogic.hardware.systemcontrol@1.0_vendor
endif


LOCAL_PRELINK_MODULE := false

ifeq ($(BOARD_COMPILE_IN_SYSTEM), true)
    LOCAL_PROPRIETARY_MODULE := false
else
    LOCAL_PROPRIETARY_MODULE := true
endif

#LOCAL_32_BIT_ONLY := true

include $(BUILD_SHARED_LIBRARY)
