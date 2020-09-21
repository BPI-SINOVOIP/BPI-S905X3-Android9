#####################################################
# amlogic video decoder firmware
####################################################
LOCAL_PATH := $(call my-dir)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
OUT_PATH := $(TARGET_OUT_VENDOR)
else
OUT_PATH := $(TARGET_OUT)/
endif


include $(CLEAR_VARS)
LOCAL_MODULE := libtee_load_video_fw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH_64 := $(OUT_PATH)/lib64/
LOCAL_MODULE_PATH_32 := $(OUT_PATH)/lib/
LOCAL_SRC_FILES_arm :=  ca/lib/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_SRC_FILES_arm64 := ca/lib64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_STRIP_MODULE := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := tee_preload_fw
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH_64 := $(OUT_PATH)/bin
LOCAL_MODULE_PATH_32 := $(OUT_PATH)/bin
LOCAL_SRC_FILES_arm := ca/bin/$(LOCAL_MODULE)
LOCAL_SRC_FILES_arm64 := ca/bin64/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := 526fc4fc-7ee6-4a12-96e3-83da9565bce8
LOCAL_MULTILIB := both
LOCAL_MODULE_SUFFIX := .ta
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(OUT_PATH)/lib/teetz
LOCAL_SRC_FILES :=  ta/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_PREBUILT_MODULE_FILE := $(PRODUCT_OUT)/optee/ta/$(LOCAL_MODULE).ta
LOCAL_STRIP_MODULE := false

ifeq ($(TARGET_ENABLE_TA_SIGN), true)
TDK_PATH := $(BOARD_AML_VENDOR_PATH)/tdk
TA_SIGN_AUTO_TOOL := $(TDK_PATH)/ta_export/scripts/sign_ta_auto.py
endif

$(LOCAL_PREBUILT_MODULE_FILE): PRIVATE_SRC_DIR := $(LOCAL_PATH)/$(LOCAL_SRC_FILES)
$(LOCAL_PREBUILT_MODULE_FILE): FORCE
	@mkdir -p $(dir $@)
ifeq ($(TARGET_ENABLE_TA_SIGN), true)
ifeq ($(TARGET_ENABLE_TA_ENCRYPT), true)
	$(TA_SIGN_AUTO_TOOL) --in=$(PRIVATE_SRC_DIR) --out=$@ --encrypt=1 --keydir=$(BOARD_AML_TDK_KEY_PATH)
else
	$(TA_SIGN_AUTO_TOOL) --in=$(PRIVATE_SRC_DIR) --out=$@ --encrypt=0 --keydir=$(BOARD_AML_TDK_KEY_PATH)
endif
else #else ifeq ($(TARGET_ENABLE_TA_SIGN), true)
	cp -uvf $(PRIVATE_SRC_DIR) $@
endif

FORCE:

include $(BUILD_PREBUILT)

