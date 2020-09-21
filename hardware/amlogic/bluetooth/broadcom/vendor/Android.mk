LOCAL_PATH := $(call my-dir)

ifneq ($(BOARD_HAVE_BLUETOOTH_BROADCOM),)

include $(CLEAR_VARS)

BDROID_DIR := $(TOP_DIR)system/bt

LOCAL_SRC_FILES := \
        src/bt_vendor_brcm.c \
        src/hardware.c \
        src/userial_vendor.c \
        src/upio.c \
        src/conf.c \
        src/FallthroughBTA.cpp \
        src/sysbridge.cpp \
        src/wole/utility.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include \
        $(TOP)/vendor/amlogic/frameworks/services \
        $(TOP)/$(BOARD_AML_VENDOR_PATH)/frameworks/services

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog \
        libbinder \
        libsystemcontrolservice \
        libutils \
        libdl
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)  -DUSE_SYS_WRITE_SERVICE=1

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_C_INCLUDES += \
       $(BDROID_DIR)/device/include

LOCAL_CFLAGS += -DO_AMLOGIC
endif

LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := broadcom

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)

ifeq ($(BLUETOOTH_INF), USB)
    include $(LOCAL_PATH)/conf/bcm_usb_bt/Android.mk
else
    include $(LOCAL_PATH)/conf/meson/Android.mk
endif

ifeq ($(TARGET_PRODUCT), full_maguro)
    include $(LOCAL_PATH)/conf/samsung/maguro/Android.mk
endif
ifeq ($(TARGET_PRODUCT), full_crespo)
    include $(LOCAL_PATH)/conf/samsung/crespo/Android.mk
endif
ifeq ($(TARGET_PRODUCT), full_crespo4g)
    include $(LOCAL_PATH)/conf/samsung/crespo4g/Android.mk
endif
ifeq ($(TARGET_PRODUCT), full_wingray)
    include $(LOCAL_PATH)/conf/moto/wingray/Android.mk
endif

endif # BOARD_HAVE_BLUETOOTH_BROADCOM
