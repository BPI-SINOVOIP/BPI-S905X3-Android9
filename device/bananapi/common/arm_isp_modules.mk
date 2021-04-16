MODS_OUT := $(shell pwd)/$(PRODUCT_OUT)/obj/lib_vendor
ARM_ISP_MODULES := $(shell pwd)/$(PRODUCT_OUT)/obj/arm_isp_modules


$(shell rm $(ARM_ISP_MODULES) -rf)
$(shell mkdir $(ARM_ISP_MODULES) -p)
ARM_ISP_DRIVERS := $(TOP)/vendor/amlogic/common/arm_isp/driver/linux/kernel
$(shell cp $(ARM_ISP_DRIVERS)/* $(ARM_ISP_MODULES) -rfa)

BOARD_VENDOR_KERNEL_MODULES += $(PRODUCT_OUT)/obj/lib_vendor/iv009_isp.ko
BOARD_VENDOR_KERNEL_MODULES += $(PRODUCT_OUT)/obj/lib_vendor/iv009_isp_iq.ko
BOARD_VENDOR_KERNEL_MODULES += $(PRODUCT_OUT)/obj/lib_vendor/iv009_isp_lens.ko
BOARD_VENDOR_KERNEL_MODULES += $(PRODUCT_OUT)/obj/lib_vendor/iv009_isp_sensor.ko

KDIR := $(shell pwd)/$(PRODUCT_OUT)/obj/KERNEL_OBJ

define arm_isp-modules
	$(MAKE) -C $(KDIR) M=$(ARM_ISP_MODULES)/v4l2_dev \
	ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(PREFIX_CROSS_COMPILE) modules

	$(MAKE) -C $(KDIR) M=$(ARM_ISP_MODULES)/subdev/iq \
        ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(PREFIX_CROSS_COMPILE) modules

	$(MAKE) -C $(KDIR) M=$(ARM_ISP_MODULES)/subdev/lens \
        ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(PREFIX_CROSS_COMPILE) modules

	$(MAKE) -C $(KDIR) M=$(ARM_ISP_MODULES)/subdev/sensor \
        ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(PREFIX_CROSS_COMPILE) modules

	find $(ARM_ISP_MODULES) -name *.ko | xargs -i cp {} ${MODS_OUT}
endef
