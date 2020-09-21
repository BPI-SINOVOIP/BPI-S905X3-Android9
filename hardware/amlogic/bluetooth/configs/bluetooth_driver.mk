KERNEL_ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
PRODUCT_OUT=out/target/product/$(TARGET_PRODUCT)
TARGET_OUT=$(PRODUCT_OUT)/obj/lib_vendor

################################################################################USB INF DRV
ifeq ($(BLUETOOTH_INF), USB)

define qca-usb-bt
	@echo "inferface is usb"
	$(MAKE) -C $(shell pwd)/$(PRODUCT_OUT)/obj/KERNEL_OBJ M=$(shell pwd)/hardware/amlogic/bluetooth/qualcomm/bt_usb ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	cp $(shell pwd)/hardware/amlogic/bluetooth/qualcomm/bt_usb/bt_usb_qcom.ko $(TARGET_OUT)/
endef

define rtk-usb-bt
	@echo "inferface is usb"
	$(MAKE) -C $(shell pwd)/$(PRODUCT_OUT)/obj/KERNEL_OBJ M=$(shell pwd)/hardware/amlogic/bluetooth/realtek/rtk_btusb ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(CROSS_COMPILE) CONFIG_BT_RTKBTUSB=m modules
	cp $(shell pwd)/hardware/amlogic/bluetooth/realtek/rtk_btusb/rtk_btusb.ko $(TARGET_OUT)/

endef

define bcm-usb-bt
	@echo "inferface is usb"
	$(MAKE) -C $(shell pwd)/$(PRODUCT_OUT)/obj/KERNEL_OBJ M=$(shell pwd)/vendor/amlogic/common/broadcom/btusb/btusb_1_6_29_1/ ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	cp $(shell pwd)/vendor/amlogic/common/broadcom/btusb/btusb_1_6_29_1/btusb.ko $(TARGET_OUT)/
endef

define mtk-usb-bt
	@echo "inferface is usb"
	$(MAKE) -C $(shell pwd)/$(PRODUCT_OUT)/obj/KERNEL_OBJ M=$(shell pwd)/hardware/amlogic/bluetooth/mtk/mtkbt/bt_driver_usb/ ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	cp $(shell pwd)/hardware/amlogic/bluetooth/mtk/mtkbt/bt_driver_usb/btmtk_usb.ko $(TARGET_OUT)/
endef

################################################################################OTHER INF DRV
else

define mtk-sdio-bt
	@echo "inferface is sdio"
	$(MAKE) -C $(shell pwd)/$(PRODUCT_OUT)/obj/KERNEL_OBJ M=$(shell pwd)/hardware/amlogic/bluetooth/mtk/mtkbt/bt_driver_sdio/ ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	cp $(shell pwd)/hardware/amlogic/bluetooth/mtk/mtkbt/bt_driver_sdio/btmtksdio.ko $(TARGET_OUT)/
endef

endif


default:
	@echo "no bt configured"

QCABT:
	@echo "qca bt configured"
	$(qca-usb-bt)

RTKBT:
	@echo "rtk bt configured"
	$(rtk-usb-bt)

BCMBT:
	@echo "bcm bt configured"
	$(bcm-usb-bt)

MTKBT:
	@echo "mtk bt configured"
	$(mtk-sdio-bt)
	$(mtk-usb-bt)
