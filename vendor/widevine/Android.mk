# widevine prebuilts only available for ARM
ifneq ($(filter arm arm64,$(TARGET_ARCH)),)
ifneq ($(DRM_BUILD_TYPE), source)
ifeq (,$(wildcard $(BOARD_WIDEVINE_TA_PATH)/widevine))
LOCAL_32_BIT_ONLY := true
include $(call all-subdir-makefiles)
endif
endif
endif # TARGET_ARCH == arm
