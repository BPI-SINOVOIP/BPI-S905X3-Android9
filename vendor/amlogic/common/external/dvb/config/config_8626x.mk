TARGET=8626x
ARCH=arc
CHIP=8626x

-include $(ROOTDIR)/config/env_8626x.mk

KERNEL_INCDIR?=/home/liuying/am_arc_linux/kernel_26/include
ROOTFS_INCDIR?=/nfsroot/liuying/rootfs_stb/include
ROOTFS_LIBDIR?=/nfsroot/liuying/rootfs_stb/lib

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
IMG_PNG=y
IMG_JPEG=y
IMG_GIF=y
IMG_TIFF=n

FONT_FREETYPE=y

LIBICONV=n

AMLOGIC_LIBPLAYER=y
