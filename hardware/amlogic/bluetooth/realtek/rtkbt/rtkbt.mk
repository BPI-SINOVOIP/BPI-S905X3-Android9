# RELEASE NAME: 20190311_BT_ANDROID_9.0
# RTKBT_API_VERSION=2.1.1.0

BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_RTK := true
BOARD_HAVE_BLUETOOTH_RTK_TV := true

ifeq ($(BOARD_HAVE_BLUETOOTH_RTK_TV), true)
#Firmware For Tv
include $(LOCAL_PATH)/Firmware/TV/TV_Firmware.mk
else
#Firmware For Tablet
include $(LOCAL_PATH)/Firmware/BT/BT_Firmware.mk
endif

BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(LOCAL_PATH)/bluetooth

PRODUCT_COPY_FILES += \
       $(LOCAL_PATH)/vendor/etc/bluetooth/rtkbt.conf:vendor/etc/bluetooth/rtkbt.conf \
       $(LOCAL_PATH)/system/etc/permissions/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml \
       $(LOCAL_PATH)/system/etc/permissions/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \

ifeq ($(BOARD_HAVE_BLUETOOTH_RTK_TV), true)
PRODUCT_COPY_FILES += \
        $(LOCAL_PATH)/vendor/usr/keylayout/Vendor_005d_Product_0001.kl:vendor/usr/keylayout/Vendor_005d_Product_0001.kl \
        $(LOCAL_PATH)/vendor/usr/keylayout/Vendor_005d_Product_0002.kl:vendor/usr/keylayout/Vendor_005d_Product_0002.kl
endif

# base bluetooth
PRODUCT_PACKAGES += \
    Bluetooth \
    libbt-vendor \
    rtkcmd \
    audio.a2dp.default \
    bluetooth.default \
    android.hardware.bluetooth@1.0-impl \
    android.hidl.memory@1.0-impl \
    android.hardware.bluetooth@1.0-service \
    android.hardware.bluetooth@1.0-service.rc \


PRODUCT_PROPERTY_OVERRIDES += persist.bluetooth.btsnoopsize=0xffff \
                    persist.bluetooth.rtkcoex=true \
                    bluetooth.enable_timeout_ms=11000
