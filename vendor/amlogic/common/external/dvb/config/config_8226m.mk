TARGET=8226m
ARCH=arm
CHIP=8226m

-include $(ROOTDIR)/config/env_8226m.mk

KERNEL_INCDIR?=/home/wanghaihua/arm/svn_kernel/am_arm_linux/kernel/include
ROOTFS_INCDIR?=/home/wanghaihua/arm/svn_kernel/am_arm_linux/build_rootfs/7266m_8626m_demo/build/staging/usr/include
ROOTFS_LIBDIR?=/home/wanghaihua/arm/arm_ref/build_rootfs/7266m_8626m_demo/build/staging/lib

DEBUG=y
MEMWATCH=y

LINUX_INPUT=y
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
IMG_PNG=y

IMG_JPEG=n

FONT_FREETYPE=y

LIBICONV=y
