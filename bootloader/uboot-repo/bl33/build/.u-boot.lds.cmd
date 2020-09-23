cmd_u-boot.lds := aarch64-none-elf-gcc -E -Wp,-MD,./.u-boot.lds.d -D__KERNEL__ -D__UBOOT__ -DCONFIG_SYS_TEXT_BASE=0x01000000   -D__ARM__           -march=armv8-a  -mstrict-align  -ffunction-sections -fdata-sections -fno-common -ffixed-r9   -fno-common -ffixed-x18 -pipe -Iinclude  -I../include -I../arch/arm/include -include ../include/linux/kconfig.h  -nostdinc -isystem /opt/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin/../lib/gcc/aarch64-none-elf/4.8.3/include -include ../include/u-boot/u-boot.lds.h -DCPUDIR=arch/arm/cpu/armv8  -ansi -D__ASSEMBLY__ -x assembler-with-cpp -P -o u-boot.lds ../arch/arm/cpu/armv8/u-boot.lds

source_u-boot.lds := ../arch/arm/cpu/armv8/u-boot.lds

deps_u-boot.lds := \
  ../include/u-boot/u-boot.lds.h \

u-boot.lds: $(deps_u-boot.lds)

$(deps_u-boot.lds):
