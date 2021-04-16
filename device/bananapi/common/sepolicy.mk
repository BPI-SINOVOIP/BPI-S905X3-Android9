BOARD_SEPOLICY_DIRS += \
    device/bananapi/common/sepolicy \
    device/bananapi/common/sepolicy/aml_core
ifneq ($(TARGET_BUILD_GOOGLE_ATV), true)
BOARD_SEPOLICY_DIRS += \
    device/google/atv/sepolicy
endif
