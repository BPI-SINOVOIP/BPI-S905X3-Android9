cmd_fs/ubifs/crc16.o := aarch64-none-elf-gcc -Wp,-MD,fs/ubifs/.crc16.o.d  -nostdinc -isystem /opt/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin/../lib/gcc/aarch64-none-elf/4.8.3/include -Iinclude  -I../include -I../arch/arm/include -include ../include/linux/kconfig.h  -I../fs/ubifs -Ifs/ubifs -D__KERNEL__ -D__UBOOT__ -DCONFIG_SYS_TEXT_BASE=0x01000000 -Wall -Wstrict-prototypes -Wno-format-security -fno-builtin -ffreestanding -Os -fno-stack-protector -g -fstack-usage -Wno-format-nonliteral -Werror -D__ARM__ -march=armv8-a -mstrict-align -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -fno-common -ffixed-x18 -pipe    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(crc16)"  -D"KBUILD_MODNAME=KBUILD_STR(crc16)" -c -o fs/ubifs/crc16.o ../fs/ubifs/crc16.c

source_fs/ubifs/crc16.o := ../fs/ubifs/crc16.c

deps_fs/ubifs/crc16.o := \
  ../include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/use/stdint.h) \
  ../include/linux/posix_types.h \
  ../include/linux/stddef.h \
  ../arch/arm/include/asm/posix_types.h \
  ../arch/arm/include/asm/types.h \
    $(wildcard include/config/arm64.h) \
  /opt/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/lib/gcc/aarch64-none-elf/4.8.3/include/stdbool.h \
  ../fs/ubifs/crc16.h \

fs/ubifs/crc16.o: $(deps_fs/ubifs/crc16.o)

$(deps_fs/ubifs/crc16.o):
