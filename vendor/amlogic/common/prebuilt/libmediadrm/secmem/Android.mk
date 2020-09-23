LOCAL_PATH:= $(call my-dir)
ifeq ($(BOARD_OMX_WITH_OPTEE_TVP),true)

ifneq ($(filter arm arm64, $(TARGET_ARCH)),)

#####################################################################
# libsecmem.so
include $(CLEAR_VARS)
LOCAL_MODULE := libsecmem
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_SRC_FILES := $(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_STRIP_MODULE := false
LOCAL_32_BIT_ONLY := true
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

# libsecmem.so
include $(CLEAR_VARS)
LOCAL_MODULE := 2c1a33c0-44cc-11e5-bc3b0002a5d5c51b
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/teetz
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .ta
ifeq ($(TARGET_ENABLE_TA_SIGN), true)
$(info $(shell mkdir -p $(shell pwd)/$(PRODUCT_OUT)/signed/secmem))
$(info $(shell $(shell pwd)/$(BOARD_AML_VENDOR_PATH)/tdk/ta_export/scripts/sign_ta_auto.py \
		--in=$(shell pwd)/$(LOCAL_PATH)/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX) \
		--out=$(shell pwd)/$(PRODUCT_OUT)/signed/secmem/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX) \
		--keydir=$(shell pwd)/$(BOARD_AML_TDK_KEY_PATH)))
LOCAL_SRC_FILES := ../../../../../../$(PRODUCT_OUT)/signed/secmem/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
else
LOCAL_SRC_FILES := $(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
endif
LOCAL_PROPRIETARY_MODULE := true
LOCAL_STRIP_MODULE := false
LOCAL_32_BIT_ONLY := true
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

endif # TARGET_ARCH == arm
endif

