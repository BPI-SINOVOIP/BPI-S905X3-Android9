# export CONFIG_MAC80211_MESH=y
# export CONFIG_MAC80211_RC_PID=y
ccflags-y += -DCONFIG_MAC80211_RC_DEFAULT_MINSTREL
ccflags-y += -DMINSTREL_SAMPLE_SKIP_LOW_RATE
ccflags-y += -DMINSTREL_SAMPLE_SKIP_LOW_RATE_HT

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
#ccflags-y += -DCHKSUM_HW_SUPPORT
ccflags-y += -DCONFIG_ATBM_APOLLO_TESTMODE
ccflags-y += -DCONFIG_NL80211_TESTMODE
ccflags-y += -DCONFIG_MAC80211_ATBM_ROAMING_CHANGES
ccflags-y += -DATBM_SUPPORT_WIDTH_40M
ifeq ($(CONFIG_MAC80211_DEBUGFS),y)
ccflags-y  += -DCONFIG_MAC80211_DEBUGFS
endif
ccflags-$(CONFIG_MAC80211_RC_MINSTREL) += -DCONFIG_MAC80211_RC_MINSTREL
ccflags-$(CONFIG_MAC80211_RC_MINSTREL_HT) += -DCONFIG_MAC80211_RC_MINSTREL_HT

obj-$(CONFIG_COMPAT_MAC80211) += mac80211.o
# mac80211 objects
mac80211-y := \
	main.o \
	status.o \
	sta_info.o \
	wep.o \
	wpa.o \
	wapi.o \
	scan.o offchannel.o \
	ht.o agg-tx.o agg-rx.o \
	ibss.o \
	mlme.o work.o \
	iface.o \
	rate.o \
	michael.o \
	tkip.o \
	aes_ccm.o \
	aes_cmac.o \
	cfg.o \
	rx.o \
	spectmgmt.o \
	tx.o \
	key.o \
	util.o \
	wme.o \
	event.o \
	chan.o	wext_cfg.o bridge.o

mac80211-$(CONFIG_MAC80211_LEDS) += led.o
mac80211-$(CONFIG_MAC80211_DEBUGFS) += \
	debugfs.o \
	debugfs_sta.o \
	debugfs_netdev.o \
	debugfs_key.o

mac80211-$(CONFIG_MAC80211_MESH) += \
	mesh.o \
	mesh_pathtbl.o \
	mesh_plink.o \
	mesh_hwmp.o

mac80211-$(CONFIG_PM) += pm.o

mac80211-$(CONFIG_MAC80211_DRIVER_API_TRACER) += driver-trace.o
CFLAGS_driver-trace.o := -I$(src)

# objects for PID algorithm
rc80211_pid-$(CONFIG_MAC80211_DEBUGFS) := rc80211_pid_algo.o
rc80211_pid-$(CONFIG_MAC80211_DEBUGFS) += rc80211_pid_debugfs.o

rc80211_minstrel-y := rc80211_minstrel.o
rc80211_minstrel-$(CONFIG_MAC80211_DEBUGFS) += rc80211_minstrel_debugfs.o

rc80211_minstrel_ht-y := rc80211_minstrel_ht.o
rc80211_minstrel_ht-$(CONFIG_MAC80211_DEBUGFS) += rc80211_minstrel_ht_debugfs.o

mac80211-$(CONFIG_MAC80211_RC_PID) += $(rc80211_pid-y)
mac80211-$(CONFIG_MAC80211_RC_MINSTREL) += $(rc80211_minstrel-y)
mac80211-$(CONFIG_MAC80211_RC_MINSTREL_HT) += $(rc80211_minstrel_ht-y)

ccflags-y += -D__CHECK_ENDIAN__
MODFLAGS =-DMODULE -fno-pic
CFLAGS_MODULE =$(MODFLAGS)
# Extra IE for probe response from upper layer is needed in P2P GO
# For offloading probe response to FW, the extra IE must be included
# in the probe response template
