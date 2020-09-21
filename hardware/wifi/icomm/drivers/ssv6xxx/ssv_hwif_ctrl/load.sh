#/bin/bash

echo "=================================================="
echo "1.Unload Module"
echo "=================================================="
./unload.sh

echo "=================================================="
echo "2.Set Hardware Capability"
echo "=================================================="
./ssvcfg.sh

echo "=================================================="
echo "3.Load MMC Module"
echo "=================================================="

modprobe ssv_mac80211
modprobe sdhci-pci
modprobe sdhci

modprobe mmc_core sdiomaxclock=25000000
modprobe mmc_block

echo "=================================================="
echo "4.Load SSV HWIF CTRL Driver"
echo "=================================================="
insmod ../6051.Q0.1009.18.000000/ssv6xxx/ssv6051.ko stacfgpath=../6051.Q0.1009.18.000000/ssv6xxx/sta.cfg
insmod ../SMAC.0000.1807.16.B5/ssv6x5x/ssv6xxx.ko tu_stacfgpath=../SMAC.0000.1807.16.B5/ssv6x5x/sta.cfg
insmod ./hwif_ctrl/sdio/ssv_hwif_ctrl.ko

