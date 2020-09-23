# widevine prebuilts only available for ARM
ifneq ($(filter arm arm64,$(TARGET_ARCH)),)
LOCAL_32_BIT_ONLY := true
ifneq ($(DRM_BUILD_TYPE), source)
include $(call all-subdir-makefiles)
endif
endif # TARGET_ARCH == arm
