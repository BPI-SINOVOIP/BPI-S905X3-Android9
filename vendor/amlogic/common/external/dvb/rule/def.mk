Q=@
#Q=
INFO=echo "    AML:"

VERSION:=1.0

ifeq ($(TARGET),)
        TARGET:=default
        CONFIG_FILE:=config.mk
else
        CONFIG_FILE:=config_$(TARGET).mk
endif

CURRDIR:=$(abspath $(CURDIR))
ROOTDIR:=$(abspath $(CURDIR)/$(BASE))

include $(ROOTDIR)/config/$(CONFIG_FILE)

ifeq ($(ROOTDIR),$(CURRDIR))
        RELPATH=
else
        RELPATH:=$(patsubst $(ROOTDIR)/%,%,$(CURRDIR))
endif

ROOT_BUILDDIR:=$(ROOTDIR)/build
TARGET_BUILDDIR:=$(ROOT_BUILDDIR)/$(TARGET)
HOST_BUILDDIR:=$(ROOT_BUILDDIR)/host

ifeq ($(RELPATH),)
        BUILDDIR:=$(TARGET_BUILDDIR)
else
        BUILDDIR:=$(TARGET_BUILDDIR)/$(RELPATH)
	HOST_BUILDDIR:=$(HOST_BUILDDIR)/$(RELPATH)
endif

MAKE=make --no-print-directory
MKDIR=mkdir -p

CFLAGS+=-Wall -I$(ROOTDIR)/include/am_adp -I$(ROOTDIR)/include/am_mw -I$(ROOTDIR)/include/am_app -I$(ROOTDIR)/include/am_ver

include $(ROOTDIR)/rule/def_$(ARCH).mk
-include $(ROOTDIR)/rule/def_$(TARGET).mk

CC:=$(CROSS_COMPILE)gcc
AR:=$(CROSS_COMPILE)ar
LD:=$(CROSS_COMPILE)ld
RANLIB:=$(CROSS_COMPILE)ranlib
STRIP:=$(CROSS_COMPILE)strip

HOST_CC:=gcc
HOST_AR:=ar
HOST_LD:=ld
HOST_RANLIB:=ranlid

