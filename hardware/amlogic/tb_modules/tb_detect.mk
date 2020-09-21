ifeq ($(KERNEL_A32_SUPPORT), true)
KERNEL_ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-
else
KERNEL_ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-
endif

#ifeq ($(KERNEL_A32_SUPPORT), true)
#KERNEL_ARCH ?= arm
#CROSS_COMPILE ?= arm-linux-gnueabihf-
#else
#KERNEL_ARCH ?= arm64
#CROSS_COMPILE ?= aarch64-linux-gnu-
#endif

DETECT_IN=hardware/amlogic/tb_modules
DETECT_OUT=$(PRODUCT_OUT)/obj/tb_modules

define tb-modules
$(TB_DETECT_KO):
	rm $(DETECT_OUT) -rf
	mkdir -p $(DETECT_OUT)
	cp $(DETECT_IN)/* $(DETECT_OUT)/ -airf
	@echo "make Amlogic TB Detect module KERNEL_ARCH is $(KERNEL_ARCH)"
	$(MAKE) -C $(shell pwd)/$(PRODUCT_OUT)/obj/KERNEL_OBJ M=$(shell pwd)/$(DETECT_OUT)/ ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(CROSS_COMPILE) CONFIG_TB_DETECT=m modules

	mkdir -p $(PRODUCT_OUT)/obj/lib_vendor
	rm $(PRODUCT_OUT)/obj/lib_vendor/tb_detect.ko -f
	cp $(DETECT_OUT)/tb_detect.ko $(PRODUCT_OUT)/obj/lib_vendor/tb_detect.ko -airf
	@echo "make Amlogic TB Detect module finished current dir is $(shell pwd)"
endef
