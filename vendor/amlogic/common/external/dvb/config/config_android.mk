TARGET=android
ARCH=arm
CHIP=8226m

-include $(ROOTDIR)/config/env_android.mk

KERNEL_INCDIR:=$(ANDROID_TOPDIR)/kernel
ROOTFS_INCDIR:=
ROOTFS_LIBDIR:=$(ANDROID_TOPDIR)/out/target/product/m1ref/system/lib
ANDROID_HOSTDIR:=$(ANDROID_TOPDIR)/out/host/linux-x86
JAVA_CLASSPATH:=./:src/:$(ANDROID_TOPDIR)/prebuilt/sdk/8/android.jar

ARMELF_X_ANDROID=$(ROOTDIR)/android/armelf.x 
ARMELF_XSC_ANDROID=$(ROOTDIR)/android/armelf.xsc 
ANDROID_SYSTEM_CORE=$(ROOTDIR)/android/ndk/system/core/

NDK_INCDIR?=$(ROOTDIR)/android/ndk/include
NDK_LIBDIR?=$(ROOTDIR)/android/ndk/lib

CRT_BEGIN_O=$(NDK_LIBDIR)/crtbegin_dynamic.o
CRT_END_O=$(NDK_LIBDIR)/crtend_android.o

EX_LIBDIR=$(ROOTDIR)/android/ex_lib
EX_INCDIR=$(ROOTDIR)/android/ex_include

ANDROID_VERSION=2.2
TARGET_PRODUCT=m1ref
TARGET_BUILD_VARIANT=eng

DEBUG=y
MEMWATCH=n

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

IMG_BMP=n
IMG_GIF=n
IMG_PNG=n

IMG_JPEG=n

FONT_FREETYPE=y

LIBICONV=n

AMLOGIC_LIBPLAYER=y

