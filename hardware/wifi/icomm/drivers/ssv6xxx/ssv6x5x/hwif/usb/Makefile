ifeq ($(KBUILD_TOP),)
    ifneq ($(KBUILD_EXTMOD),)
    KBUILD_DIR := $(KBUILD_EXTMOD)
    else
    KBUILD_DIR := $(PWD)
    endif
KBUILD_TOP := $(KBUILD_DIR)/../../
endif

include $(KBUILD_TOP)/config.mak

KBUILD_EXTRA_SYMBOLS += $(KBUILD_TOP)/ssv6200smac/Module.symvers
ifeq ($(DRV_OPT), HUW_DRV)
KBUILD_EXTRA_SYMBOLS += $(KBUILD_TOP)/hci_wrapper/Module.symvers
endif
KBUILD_EXTRA_SYMBOLS += $(KBUILD_TOP)/ssvdevice/Module.symvers

KMODULE_NAME=ssv6200_usb
KERN_SRCS := usb.c

include $(KBUILD_TOP)/rules.mak
