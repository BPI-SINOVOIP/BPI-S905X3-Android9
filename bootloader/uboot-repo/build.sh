uboot_name=bananapi_m5_v1
project_path=bananapi_m5

./mk $uboot_name --systemroot;

#cp build/u-boot.bin ../../device/bananapi/$project_path/bootloader.img;
#cp build/u-boot.bin.usb.bl2 ../../device/bananapi/$project_path/upgrade/u-boot.bin.usb.bl2;
#cp build/u-boot.bin.usb.tpl ../../device/bananapi/$project_path/upgrade/u-boot.bin.usb.tpl;
#cp build/u-boot.bin.sd.bin ../../device/bananapi/$project_path/upgrade/u-boot.bin.sd.bin;
