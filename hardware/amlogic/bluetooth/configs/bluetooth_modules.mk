####################################################################USB INF KO

ifeq ($(BLUETOOTH_INF), USB)

ifeq ($(BLUETOOTH_MODULE), RTKBT)
	BOARD_VENDOR_KERNEL_MODULES += $(PRODUCT_OUT)/obj/lib_vendor/rtk_btusb.ko
endif

ifeq ($(BLUETOOTH_MODULE), BCMBT)
	BOARD_VENDOR_KERNEL_MODULES += $(PRODUCT_OUT)/obj/lib_vendor/btusb.ko
endif

ifeq ($(BLUETOOTH_MODULE), MTKBT)
	BOARD_VENDOR_KERNEL_MODULES += $(PRODUCT_OUT)/obj/lib_vendor/btmtk_usb.ko
endif

ifeq ($(BLUETOOTH_MODULE), QCABT)
	BOARD_VENDOR_KERNEL_MODULES += $(PRODUCT_OUT)/obj/lib_vendor/bt_usb_qcom.ko
endif

####################################################################OTHER INF KO
else

ifeq ($(BLUETOOTH_MODULE), MTKBT)
	BOARD_VENDOR_KERNEL_MODULES += $(PRODUCT_OUT)/obj/lib_vendor/btmtksdio.ko
endif

endif
