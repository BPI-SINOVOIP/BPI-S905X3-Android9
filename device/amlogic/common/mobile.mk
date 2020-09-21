#for mobile network
$(call inherit-product-if-exists, external/libusb/usbmodeswitch/CopyConfigs.mk)
$(call inherit-product-if-exists, hardware/amlogic/libril/config/Copy.mk)


PRODUCT_PACKAGES += \
    modem_dongle_d \
    usb_modeswitch \
    libaml-ril.so \
    init-pppd.sh \
    rild \
    ip-up \
    chat

