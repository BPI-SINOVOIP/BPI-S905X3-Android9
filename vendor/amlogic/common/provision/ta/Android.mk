LOCAL_PATH := $(call my-dir)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
OUT_PATH := $(PRODUCT_OUT)/vendor
else
OUT_PATH := $(PRODUCT_OUT)/system
endif

include $(CLEAR_VARS)
ifeq ($(TARGET_ENABLE_TA_SIGN), true)
ifeq ($(TARGET_ENABLE_TA_ENCRYPT), true)
ENCRYPT := --encrypt=1
else
ENCRYPT := --encrypt=0
endif

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
KEYDIR := --keydir=$(shell pwd)/$(BOARD_AML_TDK_KEY_PATH)
else
KEYDIR := --keydir=$(shell pwd)/$(BOARD_AML_VENDOR_PATH)/tdk/ta_export/keys/
endif
$(info $(shell mkdir $(shell pwd)/$(LOCAL_PATH)/signed))
$(info $(shell $(shell pwd)/$(BOARD_AML_VENDOR_PATH)/tdk/ta_export/scripts/sign_ta_auto.py \
		--in=$(shell pwd)/$(LOCAL_PATH)/e92a43ab-b4c8-4450-aa12b1516259613b.ta \
		--out=$(shell pwd)/$(LOCAL_PATH)/signed/e92a43ab-b4c8-4450-aa12b1516259613b.ta \
		$(ENCRYPT) \
		$(KEYDIR)))

LOCAL_SRC_FILES := signed/e92a43ab-b4c8-4450-aa12b1516259613b.ta
else
LOCAL_SRC_FILES := e92a43ab-b4c8-4450-aa12b1516259613b.ta
endif

LOCAL_MODULE := e92a43ab-b4c8-4450-aa12b1516259613b
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .ta
LOCAL_MODULE_PATH := $(OUT_PATH)/lib/teetz
LOCAL_STRIP_MODULE := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := tee_provision_ta
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_REQUIRED_MODULES := e92a43ab-b4c8-4450-aa12b1516259613b
include $(BUILD_PHONY_PACKAGE)
