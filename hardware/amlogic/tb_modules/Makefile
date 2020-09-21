tb_detect-objs = tb_module.o
obj-$(CONFIG_TB_DETECT)	+= tb_detect.o

#ifndef KERNEL_A32_SUPPORT
#KERNEL_A32_SUPPORT := true
#endif

ifeq ($(ARCH), arm)
	TEXT = "this is 32bit"
#	ARCH ?= arm
#	CROSS_COMPILE ?= arm-linux-gnueabihf-
	tb_detect-objs += 32bit/tbff_fwalg_20190319.o_shipped
else
	TEXT = "this is 64bit"
#	ARCH ?= arm64
#	CROSS_COMPILE ?= aarch64-linux-gnu-
	tb_detect-objs += 64bit/tbff_fwalg_20190322.o_shipped
endif
$(warning $(TEXT))
KDIR ?=

tb_detect:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
	$(RM) Module.markers
	$(RM) modules.order
