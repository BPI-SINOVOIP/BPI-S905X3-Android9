# RELEASE

BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(LOCAL_PATH)/btbuild

PRODUCT_COPY_FILES += $(LOCAL_PATH)/firmware/mtk7668s/mt7668_patch_e2_hdr.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/mt7668_patch_e2_hdr.bin
PRODUCT_COPY_FILES += $(LOCAL_PATH)/bt_driver_usb/woble_setting.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/woble_setting.bin


PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
                      frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml

