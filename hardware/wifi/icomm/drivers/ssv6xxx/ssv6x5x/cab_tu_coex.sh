#!/bin/sh

syms="\
ssv_initmac \
ssv6xxx_sdio_driver \
stacfgpath \
cfgfirmwarepath \
ssv_cfg \
ssv6xxx_exit \
ssv6xxx_init \
ssv6xxx_hci_deregister \
ssv6xxx_dev_remove \
ssv6xxx_sdio_exit \
ssv6xxx_sdio_init \
ssvdevice_init \
ssvdevice_exit \
ssv6xxx_hci_exit \
ssv6xxx_hci_init \
cfg_cmds \
ssv6xxx_dev_probe \
ssv6xxx_hci_register \
generic_wifi_exit_module \
generic_wifi_init_module \
ssv\" \
"
syms2="\
ssv6xxx_sdio_probe \
ssv6xxx_sdio_remove \
ssv6xxx_sdio_suspend \
ssv6xxx_sdio_resume \
"

syms3="\
SSV6XXX_SDIO \
"

for i in $syms;
do
echo "Replacing $i to tu_$i\n"
find . -name "*.[ch]"| xargs sed -i "s/$i/tu_$i/g"
done

echo "Restoring tu_ssv_cfg.h to ssv_cfg.h\n"
find . -name "*.[ch]"| xargs sed -i "s/\"tu_ssv_cfg\.h\"/\"ssv_cfg\.h\"/g"
find . -name "*.[ch]"| xargs sed -i "s/<tu_ssv_cfg\.h>/\"ssv_cfg\.h\"/g"

for i in $syms2;
do
echo "Replacing $i to tu_$i\n"
find . -name "*.[ch]"| xargs sed -i "s/$i/tu_$i/g"
done

echo "Replacing SSV WLAN driver to TU SSV WLAN driver\n"
find . -name "*.[ch]"| xargs sed -i "s/SSV WLAN driver/TU SSV WLAN driver/g"

for i in $syms3;
do
echo "Replacing $i to TU_$i\n"
find . -name "*.[ch]"| xargs sed -i "s/$i/TU_$i/g"
done

