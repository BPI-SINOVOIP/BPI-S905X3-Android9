# RELEASE

BOARD_HAS_QCA_BT_ROME := true
BOARD_HAVE_BLUETOOTH_BLUEZ := false
QCOM_BT_USE_SIBS := false

BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(LOCAL_PATH)/btbuild

PRODUCT_COPY_FILES += hardware/amlogic/wifi/qcom/config/qca9377/bt/nvm_tlv_tf_1.1.bin:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/qca9377/ar3k/nvm_tlv_tf_1.1.bin \
	                  hardware/amlogic/wifi/qcom/config/qca9377/bt/rampatch_tlv_tf_1.1.tlv:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/qca9377/ar3k/rampatch_tlv_tf_1.1.tlv \
					  hardware/amlogic/wifi/qcom/config/qca6174/bt/nvm_tlv_3.2.bin:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/qca6174/ar3k/nvm_tlv_3.2.bin \
					  hardware/amlogic/wifi/qcom/config/qca6174/bt/rampatch_tlv_3.2.tlv:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/qca6174/ar3k/rampatch_tlv_3.2.tlv \
					  hardware/amlogic/wifi/qcom/config/qca9379/bt/rampatch_tlv_usb_npl_1.0.tlv:$(TARGET_COPY_OUT_VENDOR)/firmware/qca9379/ar3k/rampatch_tlv_usb_npl_1.0.tlv \
					  hardware/amlogic/wifi/qcom/config/qca9379/bt/nvm_tlv_usb_npl_1.0.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/qca9379/ar3k/nvm_tlv_usb_npl_1.0.bin

PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
                      frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml

PRODUCT_PROPERTY_OVERRIDES += poweroff.doubleclick=1
PRODUCT_PROPERTY_OVERRIDES += qcom.bluetooth.soc=rome_uart
PRODUCT_PROPERTY_OVERRIDES += wc_transport.soc_initialized=0

PRODUCT_PACKAGES += libbt-vendor
