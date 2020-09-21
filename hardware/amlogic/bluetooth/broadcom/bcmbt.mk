# RELEASE

BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(LOCAL_PATH)/btbuild

PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6210/BT/bcm20710a1.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM20702.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6476/GPS/bcm2076b1.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM2076.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6330/BT/bcm40183b2.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM4330.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/62x2/BT/bcm43241b4.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/bcm43241b4.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6335/BT/bcm4335c0.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/bcm4335c0.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6441/BT/bcm43341b0.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/bcm43341b0.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6212/BT/bcm43438a0.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/4343.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4354/bcm4354a1.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM4350.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/4356/bcm4356a2.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM4354.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/6255/BT/BCM4345C0.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM4345C0.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6269/BT/bcm43569a2.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/bcm43569a2.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6271S/BT/BCM43751.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM43751.hcd
ifeq ($(U212_CHIP), false)
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/BT/R3/BCM4359C0.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM4359C0.hcd
else
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6398/BT/R2/BCM4359C0.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM4359C0.hcd
endif
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/bcm43569a2.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/bcm43569a2.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6256/BT/BCM4345C5.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM4345C5.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP6236/BT/BCM4343B0.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM43430B0.hcd
PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/20704/BCM20703A2.hcd:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/BCM20703A2.hcd


PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
	                  frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml

#PRODUCT_COPY_FILES += hardware/amlogic/wifi/bcm_ampak/config/AP62x8/rc.conf:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/rc.conf
