# Specify Include folders
EXTRA_CFLAGS += -I$(src)
EXTRA_CFLAGS += -I$(src)/inc
EXTRA_CFLAGS += -I$(src)/src
EXTRA_CFLAGS += -I$(src)/src/gki

EXTRA_CFLAGS += -DEXPORT_SYMTAB

LITE ?= FALSE
ifeq ($(strip $(LITE)),TRUE)
EXTRA_CFLAGS += -DBUILDCFG
EXTRA_CFLAGS += -DBTUSB_LITE
EXTRA_CFLAGS += -I$(src)/../btpcm
EXTRA_CFLAGS += -I$(src)/../btsbc
COMPONENTS_PATH := $(src)/../../../../../Components
EXTRA_CFLAGS += -I$(COMPONENTS_PATH)/bta/include
EXTRA_CFLAGS += -I$(COMPONENTS_PATH)/stack/include
EXTRA_CFLAGS += -I$(COMPONENTS_PATH)/hcis
endif

SEC ?= FALSE
ifeq ($(strip $(SEC)),TRUE)
EXTRA_CFLAGS += -DBTUSB_LITE_SEC
EXTRA_CFLAGS += -I$(src)/../btsec
endif

EXTRA_CFLAGS += -DEXPORT_SYMTAB

# BTUSB_VID and BTUSB_PID can be defined to support a single BT controller type
# The following definitions are examples for 20702A1 chip
#EXTRA_CFLAGS += -DBTUSB_VID=0x0A5C
#EXTRA_CFLAGS += -DBTUSB_PID=0x22BE

usbobjs := \
        src/btusb.o \
        src/btusb_dev.o \
        src/btusb_isoc.o

ifeq ($(strip $(LITE)),TRUE)
liteobjs := src/btusb_lite.o \
            src/btusb_lite_av.o \
            src/btusb_lite_avdt.o \
            src/btusb_lite_l2c.o \
            src/btusb_lite_hci.o
endif

gkiobjs := \
        src/gki/gki_buffer.o \
        src/gki/gki_klinux.o

obj-m += btusb.o
btusb-objs := $(usbobjs) $(gkiobjs) $(liteobjs)

# specify Kernel build location
KDIR    := /lib/modules/$(shell uname -r)/build

PWD     := $(shell pwd)

default:
ifeq ($(strip $(LITE)),TRUE)
	cat ../btpcm/Module.symvers > ./Module.symvers
	cat ../btsbc/Module.symvers >> ./Module.symvers
ifeq ($(strip $(SEC)),TRUE)
	cat ../btsec/Module.symvers >> ./Module.symvers
endif
endif
	$(MAKE) -C $(KDIR) src=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
	rm -f Module.markers
	rm -f Module.symvers
	rm -f modules.order
	rm -f ./src/*.o
	rm -f ./src/.*.o.cmd
	rm -f ./src/gki/*.o
	rm -f ./src/gki/.*.o.cmd

help:
	@echo "BTUSB module configurations and targets:"
	@echo "  LITE=FALSE         : No StackLite (Default)"
	@echo "  LITE=TRUE          : StackLite compiled"
	@echo "  SEC=FALSE          : No SEC Encoder Support(default)"
	@echo "  SEC=TRUE           : SEC Encoder Support. Requires StackLite"
	@echo "  clean"
	
