export DIR = $(shell pwd)
#export CHIPSET = mt76x8xu

export PRODUCT_OUT := ../../../out/target/product
export KERNEL_SRC :=../../../linux-3.18

export ARM_ARCH = arm64
export LIB_ARCH = lib64

ifeq ($(LINUX_DIR),)
LINUX_SRC = $(KERNEL_SRC)
else
LINUX_SRC = $(LINUX_DIR)
endif


CFILES := \
	bt_usb_main.c \
	rome.c \
	usb_com.c \
	usb_dfu.c \
	usb_intf.c \
	usb_queue.c

MOD_NAME = bt_usb_qcom

obj-m := $(MOD_NAME).o

$(MOD_NAME)-objs := $(CFILES:.c=.o)

all: driver clean

driver:
	make ARCH=$(ARM_ARCH) CROSS_COMPILE=aarch64-linux-gnu-  -C $(LINUX_SRC) M=$(DIR) modules
	mkdir -pv $(ANDROID_PRODUCT_OUT)/system/lib64/modules/
	cp -v bt_usb_qcom.ko $(ANDROID_PRODUCT_OUT)/system/$(LIB_ARCH)/modules/
clean:
	make ARCH=$(ARM_ARCH) -C $(LINUX_SRC) M=$(DIR) clean

#EXTRA_CFLAGS=-fno-pic

#obj-m += bt_usb_qcom.o

#bt_usb_qcom-objs += \
#	bt_usb_main.o \
#	usb_com.o \
#	usb_intf.o \
#	usb_queue.o \
#	usb_dfu.o \
#	rome.o

