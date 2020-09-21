TARGET=gxb
ARCH=arm
CHIP=gxb
HOST_32=n

OUTPUT := /home/brooks/src/vmx_m9du2/m9du2-buildroot/output/mesong12a_skt_32_release
#OUTPUT:=/mnt/fileroot/hualing.chen/linux-sdk/output/mesongxl_p230_kernel49
CROSS_COMPILE:=$(OUTPUT)/host/usr/bin/arm-none-linux-gnueabi-

ifeq ($(HOST_32),y)
	OUTPUT:=/mnt/fileroot/hualing.chen/linux-sdk/output/mesongxl_p230_32_kernel49
	CROSS_COMPILE:=$(OUTPUT)/host/usr/bin/arm-linux-gnueabihf-
endif

BUILD:=$(OUTPUT)/build
KERNEL_INCDIR?=$(BUILD)/linux-amlogic-4.9-dev/include/uapi $(BUILD)/linux-amlogic-4.9-dev/include $(BUILD)/libplayer-2.1.0/amcodec/include $(BUILD)/linux-amlogic-4.9-dev/include/linux/amlogic
ROOTFS_INCDIR?=$(OUTPUT)/target/usr/include $(OUTPUT)/../../vendor/amlogic/dvb/android/ndk/include $(BUILD)/libplayer-2.1.0/amffmpeg $(BUILD)/libplayer-2.1.0/amadec/include
ROOTFS_LIBDIR?=$(OUTPUT)/target/usr/lib

DEBUG=y
MEMWATCH=n

LINUX_INPUT=n
TTY_INPUT=n

EMU_DEMUX=n

SDL_OSD=n

LINUX_DVB_FEND=y
EMU_FEND=n

EMU_SMC=n

EMU_DSC=n

EMU_AV=n

EMU_VOUT=n

IMG_BMP=y
IMG_GIF=y
IMG_PNG=n

IMG_JPEG=n

FONT_FREETYPE=n

LIBICONV=y
