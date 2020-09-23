ifneq (,$(filter true,$(BOARD_BUILD_VMX_DRM) $(BUILD_WITH_WIDEVINECAS)))
ifeq (,$(wildcard vendor/amlogic/common/frameworks/av/drmplayer))
LOCAL_PATH := $(call my-dir)


DRMPLAYER_PATH_32 := $(TARGET_OUT)/lib/


#####################################################################
# libDrmPlayer.so
include $(CLEAR_VARS)
LOCAL_MODULE := libDrmPlayer
LOCAL_MULTILIB := both
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_32 := $(DRMPLAYER_PATH_32)
LOCAL_SRC_FILES_arm := lib/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
include $(BUILD_PREBUILT)

#####################################################################
# libdrmp.so
include $(CLEAR_VARS)
LOCAL_MODULE := libdrmp
LOCAL_MULTILIB := both
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_32 := $(DRMPLAYER_PATH_32)
LOCAL_SRC_FILES_arm := lib/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
include $(BUILD_PREBUILT)

#####################################################################
# libdec_ca.so
include $(CLEAR_VARS)
LOCAL_MODULE := libdec_ca
LOCAL_MULTILIB := both
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_32 := $(DRMPLAYER_PATH_32)
LOCAL_SRC_FILES_arm := lib/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
include $(BUILD_PREBUILT)

#####################################################################
#41fe9859-71e4-4bf4-bbaad71435b127ae.ta

include $(CLEAR_VARS)
LOCAL_MODULE := 41fe9859-71e4-4bf4-bbaad71435b127ae
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/teetz
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .ta
ifeq ($(TARGET_ENABLE_TA_SIGN), true)
$(info $(shell mkdir -p $(shell pwd)/$(PRODUCT_OUT)/signed/drmplayer))
$(info $(shell $(shell pwd)/$(BOARD_AML_VENDOR_PATH)/tdk/ta_export/scripts/sign_ta_auto.py \
		--in=$(shell pwd)/$(LOCAL_PATH)/lib/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX) \
		--out=$(shell pwd)/$(PRODUCT_OUT)/signed/drmplayer/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX) \
		--keydir=$(shell pwd)/$(BOARD_AML_TDK_KEY_PATH)))
LOCAL_SRC_FILES := ../../../../../../$(PRODUCT_OUT)/signed/drmplayer/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
else
LOCAL_SRC_FILES := lib/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
endif
LOCAL_STRIP_MODULE := false
LOCAL_32_BIT_ONLY := true
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
endif
endif
