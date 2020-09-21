
KVERSION="`uname -r`"
#DRVPATH=/lib/modules/$(KVERSION)/kernel/drivers/net/wireless/ssv6200
DRVPATH=kernel/drivers/net/wireless/ssv6200
KCFLAG += -Werror
#EXTRA_CFLAGS := -I$(KBUILD_TOP) -I$(KBUILD_TOP)/include
EXTRA_CFLAGS := -I$(KBUILD_TOP) 
EXTRA_CFLAGS += -I$(KBUILD_TOP)/include 


include $(KBUILD_TOP)/config_common.mak


ccflags-y += -DREPORT_TX_STATUS_DIRECTLY
#ccflags-y += -DCAL_RX_DELAY

########################################################
## The following definition move from common mak

# use for debug
ccflags-y += -DCONFIG_IRQ_DEBUG_COUNT
ccflags-y += -DCONFIG_SSV6XXX_DEBUGFS
#### end of move from  common mak

## support 6006 fpga and 6051
ccflags-y += -DSSV_SUPPORT_SSV6006AB
ccflags-y += -DSSV_SUPPORT_SSV6051 # not define SSV_SUPPORT_HAL, please turn on this fefinition
ccflags-y += -DSSV_SUPPORT_TURISMOA

## Use crypto in SSV driver for ssv6051
#ifeq ($(findstring -DSSV_SUPPORT_SSV6051, $(ccflags-y)), -DSSV_SUPPORT_SSV6051) 
#ccflags-y += -DMULTI_THREAD_ENCRYPT
#ccflags-y += -DKTHREAD_BIND
#ccflags-y += -DUSE_LOCAL_CRYPTO
#ccflags-y += -DUSE_LOCAL_WEP_CRYPTO
#ccflags-y += -DUSE_LOCAL_TKIP_CRYPTO
#ccflags-y += -DUSE_LOCAL_CCMP_CRYPTO
#ccflags-y += -DUSE_LOCAL_SMS4_CRYPTO
#ccflags-y += -DCONFIG_SSV_WAPI
#ccflags-y += -DHAS_CRYPTO_LOCK
#ccflags-y += -DFW_WSID_WATCH_LIST
#endif

####################################################################
# mac80211 option for use local ssv_mac80211 or kernel mac80211
# if set SSV means our smac driver use local ssv_mac80211.ko
# otherwise use kernel mac80211 build-in sub-sys
####################################################################
#MAC80211_OPT = SSV
#For HUW to define some resources
ifeq ($(MAC80211_OPT), SSV)
EXTRA_CFLAGS += -I$(KBUILD_TOP)/mac80211 
EXTRA_CFLAGS += -I$(KBUILD_TOP)/mac80211/include

ccflags-y += -DSSV_MAC80211
ccflags-y += -DCONFIG_MAC80211_LEDS
ccflags-y += -DCONFIG_MAC80211_DEBUGFS
ccflags-y += -DCONFIG_MAC80211_MESH
ccflags-y += -DCONFIG_MAC80211_RC_MINSTREL
ccflags-y += -DCONFIG_MAC80211_RC_MINSTREL_HT
ccflags-y += -DCONFIG_MAC80211_RC_MINSTREL_VHT
endif
#
