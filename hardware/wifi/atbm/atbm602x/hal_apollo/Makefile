#CONFIG_USE_DMA_ADDR_BUFFER
#CONFIG_USB_AGGR_URB_TX
#CONFIG_TX_NO_CONFIRM
#
# 可以用来选择不同是模式，CONFIG_USB_AGGR_URB_TX 定义上时一定需要定义CONFIG_USE_DMA_ADDR_BUFFER
# 1> CONFIG_USB_AGGR_URB_TX + CONFIG_TX_NO_CONFIRM，每个urb 发送多个txframe 并且不需要confirm，需要打开lmac的NO_NEED_CONFIRM 宏，并且ratecontrol在lmac 实现
# 2> CONFIG_USB_AGGR_URB_TX 每个urb 发送多个txframe 并且需要confirm，需要关闭lmac的NO_NEED_CONFIRM 宏
# 3> 所有宏都不定义，每个urb 发送一个txframe 并且需要confirm，并且urb直接使用skbbuffer，需要关闭lmac的NO_NEED_CONFIRM 宏
# 4> 定义CONFIG_USE_DMA_ADDR_BUFFER，每个urb 发送一个txframe 并且需要confirm，并且urb直接DMAbuffer，需要把数据从skb copy到DMAbuffer，需要关闭lmac的NO_NEED_CONFIRM 宏，
# 5> 定义CONFIG_USE_DMA_ADDR_BUFFER+ CONFIG_TX_NO_CONFIRM，每个urb 发送一个txframe 并且不需要confirm，并且urb直接DMAbuffer，需要把数据从skb copy到DMAbuffer，需要开lmac的NO_NEED_CONFIRM 宏，并且ratecontrol在lmac 实现
# 6> 定义CONFIG_TX_NO_CONFIRM，每个urb 发送一个txframe 并且不需要confirm，并且urb直接使用skbbuffer，需要打开lmac的NO_NEED_CONFIRM 宏，并且ratecontrol在lmac 实现
#
##对于调度慢+cpu强的板子使用 模式1 或者 模式2
##对于调度快+cpu慢的板子使用 模式3 或者 模式6
## 默认使用模式 3
#####################################################################################################################################################################################################################
#ccflags-y += -DCONFIG_ATBM_APOLLO_5GHZ_SUPPORT
#ccflags-y += -DCONFIG_ATBM_APOLLO_WAPI_SUPPORT
#ccflags-y  += -I$(shell pwd)/../include/ \
        -include $(shell pwd)/../include/linux/compat-2.6.h \
        -DCOMPAT_STATIC
ATBM_MODULES_NAME ?= $(MODULES_NAME)
ifeq ($(MULT_NAME),y)
ifeq ($(USB_BUS),y)
ATBM_MODULES_NAME = $(MODULES_NAME)_usb
ATBM_FW_NAME = fw_usb
else ifeq ($(SDIO_BUS),y)
ifeq ($(CONFIG_ATBM_SDIO_40M),y)
ATBM_MODULES_NAME = $(MODULES_NAME)_sdio_40M
ATBM_FW_NAME = fw_sdio_40M
else ifeq ($(CONFIG_ATBM_SDIO_24M),y)
ATBM_MODULES_NAME = $(MODULES_NAME)_sdio_24M
ATBM_FW_NAME = fw_sdio_24M
endif
else ifeq ($(SPI_BUS),y)
ATBM_MODULES_NAME = $(MODULES_NAME)_spi
ATBM_FW_NAME = fw_spi
endif
ccflags-y += -DCONFIG_FW_NAME=\"$(ATBM_FW_NAME).bin\"
endif
ccflags-y += $(ATBM_WIFI__EXT_CCFLAGS)
ccflags-y += -DCONFIG_ATBM_APOLLO_DISABLE_BEACON_HINTS
ccflags-y += -DOLD_RATE_POLICY=1
ccflags-y += -DATBM_SUPPORT_PKG_MONITOR
ifeq ($(CONFIG_NOT_SUPPORT_40M_CHW),y)
ccflags-y += -DATBM_NOT_SUPPORT_40M_CHW
endif
ccflags-y += -DMINSTREL_RSSI_USED
ccflags-y += -DATBM_P2P_ADDR_USE_LOCAL_BIT
ccflags-y += -DATBM_WIFI_QUEUE_LOCK_BUG
ccflags-y += -DATBM_CHANGE_AP_TIMESTAMP_TO_BOOTTIME
ifneq ($(platform),PLATFORM_SUN6I)
ccflags-y += -DATBM_PM_USE_EARLYSUSPEND
else
#ccflags-y += -DATBM_SUSPEND_REMOVE_INTERFACE
#ccflags-y += -DATBM_SUPPORT_WOW
ccflags-y += -DATBM_USE_SAVED_FW
endif
#ccflags-y += -DATBM_AP_SME
#ccflags-y += -DP2P_MULTIVIF
ccflags-y += -DATBM_P2P_CHANGE
ccflags-y += -DATBM_WIFI_NO_P2P_PS
ccflags-y += -DATBM_SPECIAL_PKG_DOWN_RATE
#ccflags-y += -DMCAST_FWDING
ccflags-y += -DATBM_APOLLO_SUSPEND_RESUME_FILTER_ENABLE
ccflags-y += -DCONFIG_ATBM_IOCTRL
ccflags-y += -DOPER_CLOCK_USE_SEM
ccflags-y += -DEXIT_MODULE_RESET_USB=0
ccflags-y += -DATBM_VIF_LIST_USE_RCU_LOCK
ccflags-y += -DATBM_SUPPORT_SMARTCONFIG
#ccflags-y += -DATBM_USE_SAVED_FW
ccflags-y += -DATBM_ALLOC_MEM_DEBUG=0
ccflags-y += -DATBM_ALLOC_SKB_DEBUG=0
ifeq ($(platform),PLATFORM_AMLOGIC_S805)
ccflags-y += -DCONFIG_ATBM_APOLLO_USE_GPIO_IRQ
endif
ifeq ($(platform),PLATFORM_AMLOGIC)
#ccflags-y += -DCONFIG_ATBM_APOLLO_USE_GPIO_IRQ
endif
#ccflags-y += -DCONFIG_ATBM_APOLLO_USE_GPIO_IRQ
#ccflags-y += -DATBM_PKG_REORDER
ccflags-y += -DATBM_SUPPORT_WIDTH_40M
#ccflags-y += -DIPC_AP_USED_11G_NO_RTS
#p2p mode always in client mode, used in 4.4.2 android ,because monitor interface is not open
#(ANDRIOD_P2P in wpa_supplicant is open will cause this bug )
#ccflags-y += -DP2P_ALWAYS_CLIENT
#ccflags-y += -DATBM_11W_TEST

#wifi alliance test ,
#some function will cause pass wifi alliance certificate,but not good for some AP
#ccflags-y += -DWIFI_ALLIANCE_CERTIF
#ccflags-y += -DATBM_11W_TEST
#if used sdio download firmware
ccflags-y += -DWIFI_FW_DOWNLOAD
# Extra IE for probe response from upper layer is needed in P2P GO
# For offloading probe response to FW, the extra IE must be included
# in the probe response template
ccflags-y += -DATBM_PROBE_RESP_EXTRA_IE
ccflags-y += -DCONFIG_ATBM_APOLLO_DEBUG
#ccflags-y += -DCONFIG_MAC80211_BRIDGE
#ccflags-y += -DIPV6_FILTERING
#ccflags-y += -DCONFIG_ATBM_APOLLO_BH_DEBUG
#ccflags-y += -DCONFIG_ATBM_APOLLO_WSM_DEBUG
#ccflags-y += -DCONFIG_ATBM_APOLLO_WSM_DUMPS
#ccflags-y += -DCONFIG_ATBM_APOLLO_WSM_DUMPS_SHORT
#ccflags-y += -DCONFIG_ATBM_APOLLO_TXRX_DEBUG
#ccflags-y += -DCONFIG_ATBM_APOLLO_TX_POLICY_DEBUG
#ccflags-y += -DCONFIG_ATBM_APOLLO_STA_DEBUG
#ccflags-y += -DCONFIG_ATBM_APOLLO_DUMP_ON_ERROR
#ccflags-y += -DCONFIG_ATBM_APOLLO_ITP
ccflags-y += -DCONFIG_ATBM_APOLLO_TESTMODE
# use the mac addr in file :"/data/.mac.info"
ccflags-y += -DCUSTOM_FEATURE_MAC
#ccflags-y += -DTEST_RF_POWER
#ccflags-y += -DUSB_CMD_UES_EP0
#ccflags-y += -DRESET_CHANGE
ccflags-y += -DCONFIG_ATBMWIFI_WDT
#PLATFORM_XUNWEI
#PLATFORM_SUN6I
#PLATFORM_FRIENDLY
#PLATFORM_SUN6I_64
#ccflags-y += -DATBM_WIFI_PLATFORM=PLATFORM_HIS
ifeq ($(CONFIG_ARES),y)
ccflags-y += -DPROJ_TYPE=ARES_A
endif
ifeq ($(CONFIG_ARESB),y)
ccflags-y += -DPROJ_TYPE=ARES_B
endif

ifeq ($(CONFIG_ATHENAB),y)
ccflags-y += -DPROJ_TYPE=ATHENA_B
ifeq ($(SDIO_BUS),y)
ccflags-y += -DATBM_SDIO_PATCH
ccflags-y += -DCONFIG_TX_NO_CONFIRM
endif
endif

ifeq ($(CONFIG_FPGA),y)
#ccflags-y += -DDCXO_USE_SMU_REG
ccflags-y += -DSDIO_CHANGE=1
#ccflags-y += -DRESET_CHANGE
#ccflags-y += -DTEST_DCXO_CONFIG=1
endif
#if TEST_DCXO_CONFIG is not config,DCXO_TYPE is not used
#if TEST_DPLL_CONFIG is not config,DPLL_CLOCK is not used
ifeq ($(CONFIG_1601),y)
ccflags-y += -DDCXO_USE_SMU_REG
ccflags-y += -DSDIO_CHANGE=0
ccflags-y += -DTEST_DCXO_CONFIG
ccflags-y += -DTEST_DPLL_CONFIG
ccflags-y += -DCHIP_V1601=1
ccflags-y += -DPROJ_TYPE=APOLLO_1601
ccflags-y += -DDCXO_TYPE=CRYSTAL_MODE
ccflags-y += -DDPLL_CLOCK=DPLL_CLOCK_40M
endif
ifeq ($(CONFIG_1606),y)
ccflags-y += -DDCXO_USE_SMU_REG
ccflags-y += -DSDIO_CHANGE=0
ccflags-y += -DTEST_DCXO_CONFIG
ccflags-y += -DTEST_DPLL_CONFIG
ccflags-y += -DCHIP_V1601=0
ccflags-y += -DPROJ_TYPE=APOLLO_1606
ccflags-y += -DDCXO_TYPE=CRYSTAL_MODE
ccflags-y += -DDPLL_CLOCK=DPLL_CLOCK_40M
endif
ifeq ($(CONFIG_APOLLOC),y)
#ccflags-y += -DDCXO_USE_SMU_REG
#ccflags-y += -DTEST_DCXO_CONFIG
#ccflags-y += -DTEST_DPLL_CONFIG
ccflags-y += -DCHIP_V1601=0
ccflags-y += -DSDIO_CHANGE=1
ccflags-y += -DPROJ_TYPE=APOLLO_C
ccflags-y += -DDCXO_TYPE=CRYSTAL_MODE
ccflags-y += -DDPLL_CLOCK=DPLL_CLOCK_40M
#ccflags-y += -DCONFIG_ATBM_APOLLO_SUPPORT_SGI
endif

ifeq ($(CONFIG_APOLLOD),y)
ccflags-y += -DDCXO_USE_SMU_REG
ccflags-y += -DTEST_DCXO_CONFIG
ccflags-y += -DTEST_DPLL_CONFIG
ccflags-y += -DCHIP_V1601=0
ccflags-y += -DSDIO_CHANGE=1
ccflags-y += -DPROJ_TYPE=APOLLO_C
ccflags-y += -DDCXO_TYPE=CRYSTAL_MODE
ccflags-y += -DDPLL_CLOCK=DPLL_CLOCK_40M
#ccflags-y += -DCONFIG_ATBM_APOLLO_SUPPORT_SGI
endif
ifeq ($(CONFIG_APOLLOE),y)
ccflags-y += -DCHIP_V1601=0
ccflags-y += -DSDIO_CHANGE=1
ccflags-y += -DPROJ_TYPE=APOLLO_E
ccflags-y += -DCONFIG_ATBM_APOLLO_SUPPORT_SGI
endif
ifeq ($(CONFIG_ATHENAB_24M),y)
ccflags-y += -DDCXO_USE_SMU_REG
ccflags-y += -DTEST_DCXO_CONFIG
ccflags-y += -DTEST_DPLL_CONFIG
ccflags-y += -DPROJ_TYPE=ATHENA_B
ccflags-y += -DDCXO_TYPE=CRYSTAL_MODE
ccflags-y += -DDPLL_CLOCK=DPLL_CLOCK_24M
endif
ifeq ($(CONFIG_ATHENALITE),y)
ccflags-y += -DCHKSUM_HW_SUPPORT
ccflags-y += -DCONFIG_USB_AGGR_URB_TX
ccflags-y += -DCONFIG_USE_DMA_ADDR_BUFFER
ccflags-y += -DPROJ_TYPE=ATHENA_LITE
endif
ifeq ($(CONFIG_ATHENALITE_ECO),y)
ccflags-y += -DCHKSUM_HW_SUPPORT
ccflags-y += -DCONFIG_USB_AGGR_URB_TX
ccflags-y += -DCONFIG_USE_DMA_ADDR_BUFFER
ccflags-y += -DPROJ_TYPE=ATHENA_LITE_ECO
endif
ifeq ($(CONFIG_ARES_24M),y)
#ccflags-y += -DDCXO_USE_SMU_REG
ccflags-y += -DTEST_DCXO_CONFIG
ccflags-y += -DTEST_DPLL_CONFIG
ccflags-y += -DPROJ_TYPE=ARES_A
ccflags-y += -DDCXO_TYPE=CRYSTAL_MODE
ccflags-y += -DDPLL_CLOCK=DPLL_CLOCK_24M
endif

ccflags-y += -DCONFIG_ATBM_APOLLO_SUPPORT_SGI
ifeq ($(USB_BUS),y)
ccflags-y += -DUSB_BUS
ccflags-y += -DUSB_USE_TASTLET_TXRX
ccflags-y += -DUSB_BUS_BUG
ccflags-y += -DCONFIG_USE_DMA_ADDR_BUFFER
#ccflags-y += -DCONFIG_USB_AGGR_URB_TX
#ccflags-y += -DCONFIG_TX_NO_CONFIRM
#ccflags-y += -DATBM_IPC_ASSOC
#ccflags-y += -DATBM_PRIVATE_IE

endif
ifeq ($(SDIO_BUS),y)
ccflags-y += -DMCAST_FWDING
ccflags-y += -DRESET_CHANGE
ccflags-y += -DSDIO_BUS
#ccflags-y += -DATBM_SDIO_TXRX_ENHANCE
#ccflags-y += -DATBM_WSM_SDIO_TX_MULT
#ccflags-y += -DRESET_CHIP
endif
ifeq ($(SPI_BUS),y)
ccflags-y += -DSPI_BUS
endif
#ccflags-y += -DPS_SETUP
MODFLAGS =-DMODULE -fno-pic 
CFLAGS_MODULE =$(MODFLAGS)

ifeq ($(CONFIG_ATBM_APOLLO_DEBUGFS),y)
ccflags-y  += -DCONFIG_ATBM_APOLLO_DEBUGFS
endif
ccflags-y  += -DCONFIG_ATBM_APOLLO_DEBUG
ccflags-y += $(ATBM_WIFI__EXT_CCFLAGS)
ccflags-y += -DCONFIG_MAC80211_ATBM_RC_DEFAULT_MINSTREL
#ccflags-y += -DMINSTREL_SAMPLE_SKIP_LOW_RATE_HT
#ccflags-y += -DMINSTREL_SAMPLE_SKIP_LOW_RATE

#ccflags-y+= -DCONFIG_MAC80211_LEDS
# ccflags-y += -DCONFIG_MAC80211_NOINLINE
# ccflags-y += -DCONFIG_MAC80211_VERBOSE_DEBUG
# ccflags-y += -DCONFIG_MAC80211_HT_DEBUG
# ccflags-y += -DCONFIG_MAC80211_TKIP_DEBUG
# ccflags-y += -DCONFIG_MAC80211_IBSS_DEBUG
# ccflags-y += -DCONFIG_MAC80211_VERBOSE_PS_DEBUG
# ccflags-y += -DCONFIG_MAC80211_VERBOSE_MPL_DEBUG
# ccflags-y += -DCONFIG_MAC80211_VERBOSE_MHWMP_DEBUG
# ccflags-y += -DCONFIG_MAC80211_DEBUG_COUNTERS
ccflags-y  += -DPROBE_RESP_EXTRA_IE
#ccflags-y  += -DIPV6_FILTERING
#ccflags-y += -DCONFIG_ATBM_APOLLO_TESTMODE
#ccflags-y += -DCONFIG_NL80211_TESTMODE
ccflags-y += -DCONFIG_MAC80211_ATBM_ROAMING_CHANGES
ifeq ($(CONFIG_MAC80211_ATBM_DEBUGFS),y)
ccflags-y  += -DCONFIG_MAC80211_ATBM_DEBUGFS
endif
ccflags-$(CONFIG_MAC80211_ATBM_RC_MINSTREL) += -DCONFIG_MAC80211_ATBM_RC_MINSTREL
ccflags-$(CONFIG_MAC80211_ATBM_RC_MINSTREL_HT) += -DCONFIG_MAC80211_ATBM_RC_MINSTREL_HT
$(ATBM_MODULES_NAME)-y := \
	mac80211/bridge.o \
        mac80211/main.o \
        mac80211/status.o \
        mac80211/sta_info.o \
        mac80211/wep.o \
        mac80211/wpa.o \
        mac80211/wapi.o \
        mac80211/scan.o mac80211/offchannel.o \
        mac80211/ht.o mac80211/agg-tx.o mac80211/agg-rx.o \
        mac80211/ibss.o \
        mac80211/mlme.o mac80211/work.o \
        mac80211/iface.o \
        mac80211/rate.o \
        mac80211/michael.o \
        mac80211/tkip.o \
        mac80211/aes_ccm.o \
        mac80211/aes_cmac.o \
        mac80211/cfg.o \
        mac80211/rx.o \
        mac80211/spectmgmt.o \
        mac80211/tx.o \
        mac80211/key.o \
        mac80211/util.o \
        mac80211/wme.o \
        mac80211/event.o \
        mac80211/chan.o \
	mac80211/wext_cfg.o \
	mac80211/ap_sme.o \
	mac80211/ieee80211_atbm_mem.o \
	mac80211/ieee80211_atbm_skb.o

$(ATBM_MODULES_NAME)-$(CONFIG_MAC80211_ATBM_LEDS) += mac80211/led.o
$(ATBM_MODULES_NAME)-$(CONFIG_MAC80211_ATBM_DEBUGFS) += \
        mac80211/debugfs.o \
        mac80211/debugfs_sta.o \
        mac80211/debugfs_netdev.o \
        mac80211/debugfs_key.o

$(ATBM_MODULES_NAME)-$(CONFIG_MAC80211_ATBM_MESH) += \
        mac80211/mesh.o \
        mac80211/mesh_pathtbl.o \
        mac80211/mesh_plink.o \
        mac80211/mesh_hwmp.o

$(ATBM_MODULES_NAME)-$(CONFIG_PM) += mac80211/pm.o

$(ATBM_MODULES_NAME)-$(CONFIG_MAC80211_ATBM_DRIVER_API_TRACER) += mac80211/driver-trace.o
CFLAGS_driver-trace.o := -I$(src)

# objects for PID algorithm
rc80211_pid-$(CONFIG_MAC80211_ATBM_DEBUGFS) := mac80211/rc80211_pid_algo.o
rc80211_pid-$(CONFIG_MAC80211_ATBM_DEBUGFS) += mac80211/rc80211_pid_debugfs.o

rc80211_minstrel-y := mac80211/rc80211_minstrel.o
rc80211_minstrel-$(CONFIG_MAC80211_ATBM_DEBUGFS) += mac80211/rc80211_minstrel_debugfs.o

rc80211_minstrel_ht-y := mac80211/rc80211_minstrel_ht.o
rc80211_minstrel_ht-$(CONFIG_MAC80211_ATBM_DEBUGFS) += mac80211/rc80211_minstrel_ht_debugfs.o

$(ATBM_MODULES_NAME)-$(CONFIG_MAC80211_ATBM_RC_PID) += $(rc80211_pid-y)
$(ATBM_MODULES_NAME)-$(CONFIG_MAC80211_ATBM_RC_MINSTREL) += $(rc80211_minstrel-y)
$(ATBM_MODULES_NAME)-$(CONFIG_MAC80211_ATBM_RC_MINSTREL_HT) += $(rc80211_minstrel_ht-y)

$(ATBM_MODULES_NAME)-y += \
		fwio.o \
		txrx.o \
		queue.o \
		wsm.o \
		sta.o \
		ap.o \
		scan.o \
		main.o \
		util.o \
		dbg_event.o \
		atbm_smartconfig.o \
		smartconfig.o  \
		atbm_p2p.o
ifeq ($(USB_BUS),y)
$(ATBM_MODULES_NAME)-y += apollo_usb.o atbm_platform.o hwio_usb.o bh_usb.o
else ifeq ($(SDIO_BUS),y)
$(ATBM_MODULES_NAME)-y += apollo_sdio.o atbm_platform.o hwio_sdio.o bh_sdio.o dcxo_dpll.o
else ifeq ($(SPI_BUS),y)
$(ATBM_MODULES_NAME)-y += apollo_spi.o atbm_platform.o hwio_spi.o bh_spi.o
endif
$(ATBM_MODULES_NAME)-y	+= debug.o
$(ATBM_MODULES_NAME)-$(CONFIG_PM)		+= pm.o


obj-$(CONFIG_ATBM_APOLLO) += $(ATBM_MODULES_NAME).o

ccflags-y += -D__CHECK_ENDIAN__
MODFLAGS =-DMODULE -fno-pic
CFLAGS_MODULE =$(MODFLAGS)

