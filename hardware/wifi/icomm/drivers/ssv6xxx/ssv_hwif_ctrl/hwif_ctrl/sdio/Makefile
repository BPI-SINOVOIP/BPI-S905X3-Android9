ifeq ($(KBUILD_TOP),)
    ifneq ($(KBUILD_EXTMOD),)
    KBUILD_DIR := $(KBUILD_EXTMOD)
    else
    KBUILD_DIR := $(PWD)
    endif
KBUILD_TOP := $(KBUILD_DIR)/../../
endif

include $(KBUILD_TOP)/config.mak


KMODULE_NAME=ssv_hwif_ctrl
KERN_SRCS := sdio.c

include $(KBUILD_TOP)/rules.mak
