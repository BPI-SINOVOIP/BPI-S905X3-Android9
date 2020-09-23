cmd_drivers/mtd/ubi/crc32.o := aarch64-none-elf-gcc -Wp,-MD,drivers/mtd/ubi/.crc32.o.d  -nostdinc -isystem /opt/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin/../lib/gcc/aarch64-none-elf/4.8.3/include -Iinclude  -I../include -I../arch/arm/include -include ../include/linux/kconfig.h  -I../drivers/mtd/ubi -Idrivers/mtd/ubi -D__KERNEL__ -D__UBOOT__ -DCONFIG_SYS_TEXT_BASE=0x01000000 -Wall -Wstrict-prototypes -Wno-format-security -fno-builtin -ffreestanding -Os -fno-stack-protector -g -fstack-usage -Wno-format-nonliteral -Werror -D__ARM__ -march=armv8-a -mstrict-align -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -fno-common -ffixed-x18 -pipe    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(crc32)"  -D"KBUILD_MODNAME=KBUILD_STR(crc32)" -c -o drivers/mtd/ubi/crc32.o ../drivers/mtd/ubi/crc32.c

source_drivers/mtd/ubi/crc32.o := ../drivers/mtd/ubi/crc32.c

deps_drivers/mtd/ubi/crc32.o := \
  ../include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/use/stdint.h) \
  ../include/linux/posix_types.h \
  ../include/linux/stddef.h \
  ../arch/arm/include/asm/posix_types.h \
  ../arch/arm/include/asm/types.h \
    $(wildcard include/config/arm64.h) \
  /opt/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/lib/gcc/aarch64-none-elf/4.8.3/include/stdbool.h \
  ../arch/arm/include/asm/byteorder.h \
  ../include/linux/byteorder/little_endian.h \
  ../include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  ../include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  ../include/linux/compiler-gcc4.h \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  ../include/linux/byteorder/swab.h \
  ../include/linux/byteorder/generic.h \
  ../drivers/mtd/ubi/crc32defs.h \
  ../drivers/mtd/ubi/crc32table.h \

drivers/mtd/ubi/crc32.o: $(deps_drivers/mtd/ubi/crc32.o)

$(deps_drivers/mtd/ubi/crc32.o):
