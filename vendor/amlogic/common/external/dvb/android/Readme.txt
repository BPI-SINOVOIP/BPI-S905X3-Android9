

����������
	����arm-eabi-gcc ·�����޸�~/.bashrc PATH=$PATH:$(��İ�׿������·��)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin

����./config/env_android.mk�������������޸�����



KERNEL_INCDIR=/home/wanghaihua/arm/svn_kernel/am_arm_linux/kernel/include
ROOTFS_INCDIR=/home/stb/work/arc/am_arc_linux/ui_ref/trunk/bld_8226_h_64x2/rootfs/include/
ROOTFS_LIBDIR=/home/stb/work/arm/out/target/product/m1ref/system/lib

ARMELF_X_ANDROID=$(ROOTDIR)/android/armelf.x 
ARMELF_XSC_ANDROID=$(ROOTDIR)/android/armelf.xsc 
ANDROID_SYSTEM_CORE=$(ROOTDIR)/android/ndk/system/core/

NDK_INCDIR=$(ROOTDIR)/android/ndk/include
NDK_LIBDIR=$(ROOTDIR)/android/ndk/lib

CRT_BEGIN_O=$(NDK_LIBDIR)/crtbegin_dynamic.o
CRT_END_O=$(NDK_LIBDIR)/crtend_android.o

EX_LIBDIR=$(ROOTDIR)/android/ex_lib




