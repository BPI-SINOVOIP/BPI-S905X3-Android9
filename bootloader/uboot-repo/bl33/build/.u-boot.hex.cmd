cmd_u-boot.hex := aarch64-none-elf-objcopy  -j .text -j .rodata -j .data -j .u_boot_list -j .rela.dyn --gap-fill=0xff -O ihex u-boot u-boot.hex
