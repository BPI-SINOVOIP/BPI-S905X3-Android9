cmd_u-boot.srec := aarch64-none-elf-objcopy  -j .text -j .rodata -j .data -j .u_boot_list -j .rela.dyn --gap-fill=0xff -O srec u-boot u-boot.srec
