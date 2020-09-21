BOARD_SEPOLICY_DIRS += \
    device/amlogic/common/sepolicy \
    device/amlogic/common/sepolicy/aml_core
ifneq ($(TARGET_BUILD_GOOGLE_ATV), true)
BOARD_SEPOLICY_DIRS += \
    device/google/atv/sepolicy
endif
