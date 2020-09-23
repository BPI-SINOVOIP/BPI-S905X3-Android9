LOCAL_PATH:= $(call my-dir)

ifneq ($(filter arm arm64, $(TARGET_ARCH)),)
include $(CLEAR_VARS)
LOCAL_MODULE := libstagefright_hdcp
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_SRC_FILES := $(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_STRIP_MODULE := false
LOCAL_32_BIT_ONLY := true
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := 807798e0-f011-11e5-a5fe0002a5d5c51b
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/teetz
LOCAL_MODULE_SUFFIX := .ta
ifeq ($(TARGET_ENABLE_TA_SIGN), true)
$(info $(shell mkdir -p $(shell pwd)/$(PRODUCT_OUT)/signed/libmiracast_hdcp))
$(info $(shell $(shell pwd)/$(BOARD_AML_VENDOR_PATH)/tdk/ta_export/scripts/sign_ta_auto.py \
		--in=$(shell pwd)/$(LOCAL_PATH)/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX) \
		--out=$(shell pwd)/$(PRODUCT_OUT)/signed/libmiracast_hdcp/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX) \
		--keydir=$(shell pwd)/$(BOARD_AML_TDK_KEY_PATH)))
LOCAL_SRC_FILES := ../../../../../../$(PRODUCT_OUT)/signed/libmiracast_hdcp/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
else
LOCAL_SRC_FILES := $(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
endif
LOCAL_STRIP_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_32_BIT_ONLY := true
include $(BUILD_PREBUILT)

endif # TARGET_ARCH == arm

