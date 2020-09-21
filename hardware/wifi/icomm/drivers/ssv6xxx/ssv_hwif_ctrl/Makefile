PLATFORMS = ssv6051

KBUILD_TOP := $(PWD)/hardware/wifi/icomm/drivers/ssv6xxx/ssv_hwif_ctrl
include $(KBUILD_TOP)/$(PLATFORMS).cfg
include $(KBUILD_TOP)/platform-config.mak

PWD := $(shell pwd)

ifeq ($(KERNELRELEASE),)
# current directory is driver
CFGDIR = $(PWD)/../../../../config

-include $(CFGDIR)/build_config.cfg
-include $(CFGDIR)/rules.make

endif
EXTRA_CFLAGS := -I$(KBUILD_TOP) -I$(KBUILD_TOP)/include
KMODULE_NAME=ssv_hwif_ctrl


KBUILD_EXTRA_SYMBOLS += $(KBUILD_TOP)/../ssv6051/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(KBUILD_TOP)/../ssv6x5x/Module.symvers

KERN_SRCS := hwif_ctrl/sdio/sdio.c

#KERN_SRCS += platforms/$(PLATFORMS)-generic-wlan.c
$(KMODULE_NAME)-y += $(KERN_SRCS_S:.S=.o)
$(KMODULE_NAME)-y += $(KERN_SRCS:.c=.o)

obj-m+= $(KMODULE_NAME).o

all:module strip

module:
	make  -C $(SSV_KERNEL_PATH) ARCH=$(SSV_ARCH) CROSS_COMPILE=$(SSV_CROSS) \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" M=$(PWD) modules

install:
	install -p -m 644 $(KMODULE_NAME).ko $(KMODDESTDIR)

uninstall:
	rm -f $(KMODDESTDIR)/$(KMODULE_NAME).ko
    
strip:
	#cp eagle.ko $(KO_NAME).ko
	#cp $(KO_NAME).ko $(DEFAULT_MODULES_DIR)
	#cp ssv6200.ko $(DEFAULT_MODULES_DIR)
	cp platforms/$(PLATFORMS)-wifi.cfg image/$(KMODULE_NAME)-wifi.cfg
	cp $(KMODULE_NAME).ko image/$(KMODULE_NAME).ko
	cp platforms/cli image
ifneq ($(SSV_STRIP),)
	cp $(KMODULE_NAME).ko image/$(KMODULE_NAME)_ori.ko
	$(SSV_STRIP) --strip-unneeded image/$(KMODULE_NAME).ko
	#$(SSV_STRIP) --strip-debug image/$(KMODULE_NAME).ko
endif

clean:
	make -C $(SSV_KERNEL_PATH) ARCH=$(SSV_ARCH) CROSS_COMPILE=$(SSV_CROSS) \
	M=$(PWD) clean

