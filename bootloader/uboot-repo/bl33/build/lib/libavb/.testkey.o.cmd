cmd_lib/libavb/testkey.o := aarch64-none-elf-gcc -Wp,-MD,lib/libavb/.testkey.o.d  -nostdinc -isystem /opt/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin/../lib/gcc/aarch64-none-elf/4.8.3/include -Iinclude  -I../include -I../arch/arm/include -include ../include/linux/kconfig.h  -I../lib/libavb -Ilib/libavb -D__KERNEL__ -D__UBOOT__ -DCONFIG_SYS_TEXT_BASE=0x01000000 -Wall -Wstrict-prototypes -Wno-format-security -fno-builtin -ffreestanding -Os -fno-stack-protector -g -fstack-usage -Wno-format-nonliteral -Werror -DAVB_COMPILATION -DAVB_ENABLE_DEBUG -D__ARM__ -march=armv8-a -mstrict-align -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -fno-common -ffixed-x18 -pipe    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(testkey)"  -D"KBUILD_MODNAME=KBUILD_STR(testkey)" -c -o lib/libavb/testkey.o ../lib/libavb/testkey.c

source_lib/libavb/testkey.o := ../lib/libavb/testkey.c

deps_lib/libavb/testkey.o := \

lib/libavb/testkey.o: $(deps_lib/libavb/testkey.o)

$(deps_lib/libavb/testkey.o):
