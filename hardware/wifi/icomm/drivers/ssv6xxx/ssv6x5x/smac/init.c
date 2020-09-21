/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/nl80211.h>
#include <linux/kthread.h>
#include <linux/etherdevice.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0)
#include <crypto/hash.h>
#else
#include <linux/crypto.h>
#endif
#include <linux/rtnetlink.h>
#include <ssv6200.h>
#include <hci/hctrl.h>
#include <ssv_version.h>
#include <ssv_firmware_version.h>
#include "ssv_skb.h"
#include "dev_tbl.h"
#include "dev.h"
#include "lib.h"
#include "ssv_rc.h"
#include "ssv_rc_minstrel.h"
#include "ap.h"
#include "efuse.h"
#include "init.h"
#include "ssv_skb.h"
#include "ssv_cli.h"
#include <hal.h>
#include <linux_80211.h>
#ifdef CONFIG_SSV_SUPPORT_ANDROID
#include "ssv_pm.h"
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
#include "linux_2_6_35.h"
#endif
#ifdef CONFIG_SSV6XXX_DEBUGFS
#include "ssv6xxx_debugfs.h"
#endif
#ifdef MULTI_THREAD_ENCRYPT
#include <linux/cpu.h>
#include <linux/notifier.h>
#endif
MODULE_AUTHOR("iComm-semi, Ltd");
MODULE_DESCRIPTION("Support for SSV6xxx wireless LAN cards.");
MODULE_SUPPORTED_DEVICE("SSV6xxx 802.11n WLAN cards");
MODULE_LICENSE("Dual BSD/GPL");
static const struct ieee80211_iface_limit ssv6xxx_p2p_limits[] = {
 {
  .max = 2,
  .types = BIT(NL80211_IFTYPE_STATION),
 },
 {
  .max = 1,
  .types = BIT(NL80211_IFTYPE_P2P_GO) |
       BIT(NL80211_IFTYPE_P2P_CLIENT) |
    BIT(NL80211_IFTYPE_AP),
 },
};
static const struct ieee80211_iface_combination
ssv6xxx_iface_combinations_p2p[] = {
 { .num_different_channels = 1,
   .max_interfaces = SSV6200_MAX_VIF,
   .beacon_int_infra_match = true,
   .limits = ssv6xxx_p2p_limits,
   .n_limits = ARRAY_SIZE(ssv6xxx_p2p_limits),
 },
};
#define CHAN2G(_freq,_idx) { \
    .band = INDEX_80211_BAND_2GHZ, \
    .center_freq = (_freq), \
    .hw_value = (_idx), \
    .max_power = 20, \
}
#define CHAN5G(_freq,_idx) { \
    .band = INDEX_80211_BAND_5GHZ, \
    .center_freq = (_freq), \
    .hw_value = (_idx), \
    .max_power = 20, \
}
#ifndef WLAN_CIPHER_SUITE_SMS4
#define WLAN_CIPHER_SUITE_SMS4 0x00147201
#endif
#define SHPCHECK(__hw_rate,__flags) \
    ((__flags & IEEE80211_RATE_SHORT_PREAMBLE) ? (__hw_rate +3 ) : 0)
#define RATE(_bitrate,_hw_rate,_flags) { \
    .bitrate = (_bitrate), \
    .flags = (_flags), \
    .hw_value = (_hw_rate), \
    .hw_value_short = SHPCHECK(_hw_rate,_flags) \
}
extern struct ssv6xxx_cfg tu_ssv_cfg;
static void ssv6xxx_stop_all_running_threads(struct ssv_softc *sc) ;
static const struct ieee80211_channel ssv6200_2ghz_chantable[] =
{
    CHAN2G(2412, 1 ),
    CHAN2G(2417, 2 ),
    CHAN2G(2422, 3 ),
    CHAN2G(2427, 4 ),
    CHAN2G(2432, 5 ),
    CHAN2G(2437, 6 ),
    CHAN2G(2442, 7 ),
    CHAN2G(2447, 8 ),
    CHAN2G(2452, 9 ),
    CHAN2G(2457, 10),
    CHAN2G(2462, 11),
    CHAN2G(2467, 12),
    CHAN2G(2472, 13),
    CHAN2G(2484, 14),
};
static struct ieee80211_channel ssv6200_5ghz_chantable[] = {
 CHAN5G(5180, 36),
 CHAN5G(5200, 40),
 CHAN5G(5220, 44),
 CHAN5G(5240, 48),
 CHAN5G(5260, 52),
 CHAN5G(5280, 56),
 CHAN5G(5300, 60),
 CHAN5G(5320, 64),
 CHAN5G(5500, 100),
 CHAN5G(5520, 104),
 CHAN5G(5540, 108),
 CHAN5G(5560, 112),
 CHAN5G(5580, 116),
 CHAN5G(5600, 120),
 CHAN5G(5620, 124),
 CHAN5G(5640, 128),
 CHAN5G(5660, 132),
 CHAN5G(5680, 136),
 CHAN5G(5700, 140),
 CHAN5G(5720, 144),
 CHAN5G(5745, 149),
 CHAN5G(5765, 153),
 CHAN5G(5785, 157),
 CHAN5G(5805, 161),
 CHAN5G(5825, 165),
};

static struct ieee80211_channel ssv6200_5ghz_chantable_no_midband[] = {
 CHAN5G(5180, 36),
 CHAN5G(5200, 40),
 CHAN5G(5220, 44),
 CHAN5G(5240, 48),
 CHAN5G(5260, 52),
 CHAN5G(5280, 56),
 CHAN5G(5300, 60),
 CHAN5G(5320, 64),
 CHAN5G(5745, 149),
 CHAN5G(5765, 153),
 CHAN5G(5785, 157),
 CHAN5G(5805, 161),
 CHAN5G(5825, 165),
};


static struct ieee80211_rate ssv6200_legacy_rates[] =
{
    RATE(10, 0x00, 0),
    RATE(20, 0x01, 0),
    RATE(55, 0x02, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(110, 0x03, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(60, 0x07, 0),
    RATE(90, 0x08, 0),
    RATE(120, 0x09, 0),
    RATE(180, 0x0a, 0),
    RATE(240, 0x0b, 0),
    RATE(360, 0x0c, 0),
    RATE(480, 0x0d, 0),
    RATE(540, 0x0e, 0),
};
#if ((defined CONFIG_SSV_CABRIO_E) && !(defined SSV_SUPPORT_HAL))
struct ssv6xxx_ch_cfg ch_cfg_z[] = {
 {ADR_ABB_REGISTER_1, 0, 0x151559fc},
 {ADR_LDO_REGISTER, 0, 0x00eb7c1c},
 {ADR_RX_ADC_REGISTER, 0, 0x20d000d2}
};
struct ssv6xxx_ch_cfg ch_cfg_p[] = {
 {ADR_ABB_REGISTER_1, 0, 0x151559fc},
 {ADR_RX_ADC_REGISTER, 0, 0x20d000d2}
};
int ssv6xxx_do_iq_calib(struct ssv_hw *sh, struct ssv6xxx_iqk_cfg *p_cfg)
{
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    int ret = 0;
    printk("# Do init_cali (iq)\n");
    skb = ssv_skb_alloc(sh->sc, HOST_CMD_HDR_LEN + IQK_CFG_LEN + PHY_SETTING_SIZE + RF_SETTING_SIZE);
    if (skb == NULL) {
        printk("init ssv6xxx_do_iq_calib fail!!!\n");
    }
    if ((PHY_SETTING_SIZE > MAX_PHY_SETTING_TABLE_SIZE) ||
        (RF_SETTING_SIZE > MAX_RF_SETTING_TABLE_SIZE)) {
        printk("Please recheck RF or PHY table size!!!\n");
        BUG_ON(1);
    }
    skb->data_len = HOST_CMD_HDR_LEN + IQK_CFG_LEN + PHY_SETTING_SIZE + RF_SETTING_SIZE;
    skb->len = skb->data_len;
    host_cmd = (struct cfg_host_cmd *)skb->data;
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_INIT_CALI;
    host_cmd->len = skb->data_len;
    p_cfg->phy_tbl_size = PHY_SETTING_SIZE;
    p_cfg->rf_tbl_size = RF_SETTING_SIZE;
    memcpy(host_cmd->dat32, p_cfg, IQK_CFG_LEN);
    memcpy(host_cmd->dat8+IQK_CFG_LEN, phy_setting, PHY_SETTING_SIZE);
    memcpy(host_cmd->dat8+IQK_CFG_LEN+PHY_SETTING_SIZE, ssv6200_rf_tbl, RF_SETTING_SIZE);
    HCI_SEND_CMD(sh, skb);
    ssv_skb_free(sh->sc, skb);
    {
    u32 timeout;
    sh->sc->iq_cali_done = IQ_CALI_RUNNING;
    set_current_state(TASK_INTERRUPTIBLE);
    timeout = wait_event_interruptible_timeout(sh->sc->fw_wait_q,
                                               sh->sc->iq_cali_done,
                                               msecs_to_jiffies(500));
    set_current_state(TASK_RUNNING);
    if (timeout == 0)
        return -ETIME;
    if (sh->sc->iq_cali_done != IQ_CALI_OK)
        return (-1);
    }
    return ret;
}
#endif
#define HT_CAP_RX_STBC_ONE_STREAM 0x1
#ifdef CONFIG_SSV_WAPI
static const u32 ssv6xxx_cipher_suites[] = {
    WLAN_CIPHER_SUITE_WEP40,
    WLAN_CIPHER_SUITE_WEP104,
    WLAN_CIPHER_SUITE_TKIP,
    WLAN_CIPHER_SUITE_CCMP,
    WLAN_CIPHER_SUITE_SMS4,
    WLAN_CIPHER_SUITE_AES_CMAC
};
#endif
#if defined(CONFIG_PM) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
static const struct wiphy_wowlan_support wowlan_support = {
#ifdef SSV_WAKEUP_HOST
    .flags = WIPHY_WOWLAN_ANY,
#else
    .flags = WIPHY_WOWLAN_DISCONNECT,
#endif
    .n_patterns = 0,
    .pattern_max_len = 0,
    .pattern_min_len = 0,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
    .max_pkt_offset = 0,
#endif
};
#endif
void ssv6xxx_rc_algorithm(struct ssv_softc *sc)
{
 struct ieee80211_hw *hw = sc->hw;
 hw->rate_control_algorithm = "ssv6xxx_rate_control";
}
void ssv6xxx_set_80211_hw_rate_config(struct ssv_softc *sc)
{
 struct ieee80211_hw *hw = sc->hw;
 hw->max_rates = 4;
 hw->max_rate_tries = HW_MAX_RATE_TRIES;
}
static void ssv6xxx_set_80211_hw_capab(struct ssv_softc *sc)
{
    struct ieee80211_hw *hw=sc->hw;
    struct ssv_hw *sh=sc->sh;
    struct ieee80211_sta_ht_cap *ht_info;
#ifdef CONFIG_SSV_WAPI
    hw->wiphy->cipher_suites = ssv6xxx_cipher_suites;
    hw->wiphy->n_cipher_suites = ARRAY_SIZE(ssv6xxx_cipher_suites);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,2,0)
    hw->flags = IEEE80211_HW_SIGNAL_DBM;
#else
    ieee80211_hw_set(hw, SIGNAL_DBM);
#endif
#ifdef CONFIG_SSV_SUPPORT_ANDROID
#endif
 SSV_RC_ALGORITHM(sc);
    ht_info = &sc->sbands[INDEX_80211_BAND_2GHZ].ht_cap;
    ampdu_db_log("sh->cfg.hw_caps = 0x%x\n", sh->cfg.hw_caps);
    if (sh->cfg.hw_caps & SSV6200_HW_CAP_HT) {
        if (sh->cfg.hw_caps & SSV6200_HW_CAP_AMPDU_RX)
        {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,2,0)
            hw->flags |= IEEE80211_HW_AMPDU_AGGREGATION;
            hw->flags |= IEEE80211_HW_SPECTRUM_MGMT;
            ampdu_db_log("set IEEE80211_HW_AMPDU_AGGREGATION(0x%x)\n", ((hw->flags)&IEEE80211_HW_AMPDU_AGGREGATION));
#else
            ieee80211_hw_set(hw, AMPDU_AGGREGATION);
            ieee80211_hw_set(hw, SPECTRUM_MGMT);
            ampdu_db_log("set IEEE80211_HW_AMPDU_AGGREGATION(%d)\n", ieee80211_hw_check(hw, AMPDU_AGGREGATION));
#endif
        }
        ht_info->cap |= IEEE80211_HT_CAP_SM_PS;
        if (sh->cfg.hw_caps & SSV6200_HW_CAP_GF)
            ht_info->cap |= IEEE80211_HT_CAP_GRN_FLD;
        if (sh->cfg.hw_caps & SSV6200_HW_CAP_STBC)
            ht_info->cap |= HT_CAP_RX_STBC_ONE_STREAM << IEEE80211_HT_CAP_RX_STBC_SHIFT;
        if (sh->cfg.hw_caps & SSV6200_HW_CAP_HT40)
            ht_info->cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
        if (sh->cfg.hw_caps & SSV6200_HW_CAP_SGI) {
            ht_info->cap |= IEEE80211_HT_CAP_SGI_20;
            if (sh->cfg.hw_caps & SSV6200_HW_CAP_HT40) {
                ht_info->cap |= IEEE80211_HT_CAP_SGI_40;
            }
        }
        ht_info->ampdu_factor = IEEE80211_HT_MAX_AMPDU_32K;
        ht_info->ampdu_density = IEEE80211_HT_MPDU_DENSITY_8;
        memset(&ht_info->mcs, 0, sizeof(ht_info->mcs));
        ht_info->mcs.rx_mask[0] = 0xff;
        ht_info->mcs.tx_params |= IEEE80211_HT_MCS_TX_DEFINED;
     ht_info->mcs.rx_highest = cpu_to_le16(SSV6200_RX_HIGHEST_RATE);
        ht_info->ht_supported = true;
    }
    hw->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
    if (sh->cfg.hw_caps & SSV6200_HW_CAP_P2P) {
        hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_P2P_CLIENT);
        hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_P2P_GO);
        hw->wiphy->iface_combinations = ssv6xxx_iface_combinations_p2p;
  hw->wiphy->n_iface_combinations = ARRAY_SIZE(ssv6xxx_iface_combinations_p2p);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0)
        hw->wiphy->flags |= WIPHY_FLAG_ENFORCE_COMBINATIONS;
#endif
    }
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,5,0)
    hw->wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
#endif
    if (sh->cfg.hw_caps & SSV6200_HW_CAP_AP){
        hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_AP);
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,1,0)
        hw->wiphy->flags |= WIPHY_FLAG_AP_UAPSD;
#endif
    }
    if (sh->cfg.hw_caps & SSV6200_HW_CAP_TDLS){
      hw->wiphy->flags |= WIPHY_FLAG_SUPPORTS_TDLS;
      hw->wiphy->flags |= WIPHY_FLAG_TDLS_EXTERNAL_SETUP;
      printk("TDLS function enabled in sta.cfg\n");
    }
    hw->queues = 4;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,14,0)
 hw->channel_change_time = 5000;
#endif
 hw->max_listen_interval = 1;
 SSV_SET_80211HW_RATE_CONFIG(sc);
    hw->extra_tx_headroom = TXPB_OFFSET + AMPDU_DELIMITER_LEN;
    if (sizeof(struct ampdu_hdr_st) > SSV_SKB_info_size)
        hw->extra_tx_headroom += sizeof(struct ampdu_hdr_st);
    else
        hw->extra_tx_headroom += SSV_SKB_info_size;
    if (sh->cfg.hw_caps & SSV6200_HW_CAP_2GHZ) {
  hw->wiphy->bands[INDEX_80211_BAND_2GHZ] =
   &sc->sbands[INDEX_80211_BAND_2GHZ];
    }
    if (sh->cfg.hw_caps & SSV6200_HW_CAP_5GHZ) {
        memcpy(&sc->sbands[INDEX_80211_BAND_5GHZ].ht_cap, ht_info,
            sizeof(struct ieee80211_sta_ht_cap));
  hw->wiphy->bands[INDEX_80211_BAND_5GHZ] =
   &sc->sbands[INDEX_80211_BAND_5GHZ];
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
    hw->max_rx_aggregation_subframes = sh->cfg.max_rx_aggr_size;
    hw->max_tx_aggregation_subframes = 64;
#endif
    hw->sta_data_size = sizeof(struct ssv_sta_priv_data);
    hw->vif_data_size = sizeof(struct ssv_vif_priv_data);
    memcpy(sh->maddr[0].addr, &sh->cfg.maddr[0][0], ETH_ALEN);
    hw->wiphy->addresses = sh->maddr;
    hw->wiphy->n_addresses = 1;
    if (sh->cfg.hw_caps & SSV6200_HW_CAP_P2P) {
        int i;
        for (i = 1; i < SSV6200_MAX_HW_MAC_ADDR; i++) {
      memcpy(sh->maddr[i].addr, sh->maddr[i-1].addr,
             ETH_ALEN);
      sh->maddr[i].addr[5]++;
      hw->wiphy->n_addresses++;
     }
    }
    if (!is_zero_ether_addr(sh->cfg.maddr[1]))
    {
        memcpy(sh->maddr[1].addr, sh->cfg.maddr[1], ETH_ALEN);
        if (hw->wiphy->n_addresses < 2)
            hw->wiphy->n_addresses = 2;
    }
#if defined(CONFIG_PM) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
    hw->wiphy->wowlan = wowlan_support;
#else
    hw->wiphy->wowlan = &wowlan_support;
#endif
#endif
}
#ifdef MULTI_THREAD_ENCRYPT
int ssv6xxx_cpu_callback(struct notifier_block *nfb,
                         unsigned long action,
                         void *hcpu)
{
    struct ssv_softc *sc = container_of(nfb, struct ssv_softc, cpu_nfb);
    int hotcpu = (unsigned long)hcpu;
    struct ssv_encrypt_task_list *ta = NULL;
    switch (action) {
    case CPU_UP_PREPARE:
    case CPU_UP_PREPARE_FROZEN:
    case CPU_DOWN_FAILED: {
            int cpu = 0;
            list_for_each_entry(ta, &sc->encrypt_task_head, list)
            {
                if(cpu == hotcpu)
                    break;
                cpu++;
            }
            if(ta->encrypt_task->state & TASK_UNINTERRUPTIBLE)
            {
                kthread_bind(ta->encrypt_task, hotcpu);
            }
            printk("encrypt_task %p state is %ld\n", ta->encrypt_task, ta->encrypt_task->state);
            break;
        }
    case CPU_ONLINE:
    case CPU_ONLINE_FROZEN: {
            int cpu = 0;
            list_for_each_entry(ta, &sc->encrypt_task_head, list)
            {
                if(cpu == hotcpu)
                {
                    ta->cpu_offline = 0;
                    if ( (ta->started == 0) && (cpu_online(cpu)) )
                    {
                        wake_up_process(ta->encrypt_task);
                        ta->started = 1;
                        printk("wake up encrypt_task %p state is %ld, cpu = %d\n", ta->encrypt_task, ta->encrypt_task->state, cpu);
                    }
                    break;
                }
                cpu++;
            }
            printk("encrypt_task %p state is %ld\n", ta->encrypt_task, ta->encrypt_task->state);
            break;
        }
#ifdef CONFIG_HOTPLUG_CPU
    case CPU_UP_CANCELED:
    case CPU_UP_CANCELED_FROZEN:
    case CPU_DOWN_PREPARE: {
            int cpu = 0;
            list_for_each_entry(ta, &sc->encrypt_task_head, list)
            {
                if(cpu == hotcpu)
                {
                    ta->cpu_offline = 1;
                    break;
                }
                cpu++;
            }
            printk("p = %p\n",ta->encrypt_task);
            break;
        }
    case CPU_DEAD:
    case CPU_DEAD_FROZEN: {
            break;
        }
#endif
    }
    return NOTIFY_OK;
}
#endif
static void ssv6xxx_preload_sw_cipher(void)
{
#ifdef USE_LOCAL_CRYPTO
    struct crypto_blkcipher *tmpblkcipher;
    struct crypto_cipher *tmpcipher;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0)
    struct crypto_ahash *tmphash;
#else
    struct crypto_hash *tmphash;
#endif
    printk("Pre-load cipher\n");
    tmpblkcipher = crypto_alloc_blkcipher("ecb(arc4)", 0, CRYPTO_ALG_ASYNC);
 if (IS_ERR(tmpblkcipher)) {
    printk(" ARC4 cipher allocate fail \n");
 } else {
    crypto_free_blkcipher(tmpblkcipher);
    }
 tmpcipher = crypto_alloc_cipher("aes", 0, CRYPTO_ALG_ASYNC);
 if (IS_ERR(tmpcipher)) {
     printk(" aes cipher allocate fail \n");
 } else {
    crypto_free_cipher(tmpcipher);
 }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0)
 tmphash =crypto_alloc_ahash("michael_mic", 0, CRYPTO_ALG_ASYNC);
#else
 tmphash =crypto_alloc_hash("michael_mic", 0, CRYPTO_ALG_ASYNC);
#endif
 if (IS_ERR(tmphash)) {
     printk(" mic hash allocate fail \n");
 } else {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0)
 crypto_free_ahash(tmphash);
#else
 crypto_free_hash(tmphash);
#endif
 }
#endif
}
static int tu_ssv6xxx_init_softc(struct ssv_softc *sc)
{
    void *channels;
    int ret=0;
#ifdef MULTI_THREAD_ENCRYPT
    unsigned int cpu;
#endif
    mutex_init(&sc->mutex);
    mutex_init(&sc->mem_mutex);
    sc->config_wq= create_singlethread_workqueue("ssv6xxx_cong_wq");
    INIT_WORK(&sc->hw_restart_work, ssv6xxx_restart_hw);
    INIT_WORK(&sc->set_tim_work, ssv6200_set_tim_work);
    INIT_WORK(&sc->beacon_miss_work, ssv6xxx_beacon_miss_work);
    INIT_WORK(&sc->bcast_start_work, ssv6200_bcast_start_work);
    INIT_DELAYED_WORK(&sc->bcast_stop_work, ssv6200_bcast_stop_work);
    INIT_DELAYED_WORK(&sc->bcast_tx_work, ssv6200_bcast_tx_work);
    INIT_WORK(&sc->set_ampdu_rx_add_work, ssv6xxx_set_ampdu_rx_add_work);
    INIT_WORK(&sc->set_ampdu_rx_del_work, ssv6xxx_set_ampdu_rx_del_work);
#ifdef CONFIG_SSV_SUPPORT_ANDROID
#ifdef CONFIG_HAS_EARLYSUSPEND
    sc->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 20;
    sc->early_suspend.suspend = ssv6xxx_early_suspend;
    sc->early_suspend.resume = ssv6xxx_late_resume;
    register_early_suspend(&sc->early_suspend);
#endif
    ssv_wakelock_init(sc);
#endif
    sc->mac_deci_tbl = sta_deci_tbl;
    memset((void *)&sc->tx, 0, sizeof(struct ssv_tx));
    sc->tx.hw_txqid[WMM_AC_VO] = 3; sc->tx.ac_txqid[3] = WMM_AC_VO;
    sc->tx.hw_txqid[WMM_AC_VI] = 2; sc->tx.ac_txqid[2] = WMM_AC_VI;
    sc->tx.hw_txqid[WMM_AC_BE] = 1; sc->tx.ac_txqid[1] = WMM_AC_BE;
    sc->tx.hw_txqid[WMM_AC_BK] = 0; sc->tx.ac_txqid[0] = WMM_AC_BK;
    INIT_LIST_HEAD(&sc->tx.ampdu_tx_que);
    spin_lock_init(&sc->tx.ampdu_tx_que_lock);
    memset((void *)&sc->rx, 0, sizeof(struct ssv_rx));
    spin_lock_init(&sc->rx.rxq_lock);
    skb_queue_head_init(&sc->rx.rxq_head);
    sc->rx.rx_buf = ssv_skb_alloc(sc, MAX_FRAME_SIZE_DMG);
    if (sc->rx.rx_buf == NULL)
        return -ENOMEM;
 memset(&sc->bcast_txq, 0, sizeof(struct ssv6xxx_bcast_txq));
 spin_lock_init(&sc->bcast_txq.txq_lock);
 skb_queue_head_init(&sc->bcast_txq.qhead);
 spin_lock_init(&sc->ps_state_lock);
 spin_lock_init(&sc->tx_pkt_run_no_lock);
 init_rwsem(&sc->sta_info_sem);
#ifdef CONFIG_P2P_NOA
 spin_lock_init(&sc->p2p_noa.p2p_config_lock);
#endif

#ifdef CONFIG_SSV_CUSTOM_DOMAIN
 if (sc->sh->cfg.hw_caps & SSV6200_HW_CAP_2GHZ) {
       u32 domain_2ghz_channels_size = 0;
       channels = kmalloc(sizeof(ssv6200_2ghz_chantable), GFP_KERNEL);
    if (!channels) {
     goto err_create_channel_list;
          }
     printk(" sc->sh->cfg.domain = %d\n", sc->sh->cfg.domain);
     switch (sc->sh->cfg.domain)
     {
        case DOMAIN_FCC: // 2.412 ~ 2.462 GHz, 11 channels
        case DOMAIN_North_America:
        case DOMAIN_Taiwan:
           domain_2ghz_channels_size = 11;
           memcpy(channels, &(ssv6200_2ghz_chantable[0]), sizeof(struct ieee80211_channel) * domain_2ghz_channels_size);
           break;
           
        case DOMAIN_china: // 2.412 ~ 2.472 GHz, 13 channels
        case DOMAIN_Singapore:
        case DOMAIN_ETSI:
           domain_2ghz_channels_size = 13;
           memcpy(channels, &(ssv6200_2ghz_chantable[0]), sizeof(struct ieee80211_channel) * domain_2ghz_channels_size);
           break;
           
        case DOMAIN_Japan: // 2.412 ~ 2.484 GHz, 14 channels
        case DOMAIN_Japan2:
        case DOMAIN_Korea:
            domain_2ghz_channels_size = 14;
            memcpy(channels, &(ssv6200_2ghz_chantable[0]), sizeof(struct ieee80211_channel) * domain_2ghz_channels_size);
            break;
           
        case DOMAIN_Israel: // 2.432 ~ 2.472 GHz, 9 channels
           domain_2ghz_channels_size = 9;
           memcpy(channels, &(ssv6200_2ghz_chantable[4]), sizeof(struct ieee80211_channel) * domain_2ghz_channels_size);
           break;
           
        default: // 2.412 ~ 2.484 GHz, 14 channels
           domain_2ghz_channels_size = 14;
           memcpy(channels, &(ssv6200_2ghz_chantable[0]), sizeof(struct ieee80211_channel) * domain_2ghz_channels_size);
           break;
        }
           
       sc->sbands[INDEX_80211_BAND_2GHZ].channels = channels;
       sc->sbands[INDEX_80211_BAND_2GHZ].band = INDEX_80211_BAND_2GHZ;
       sc->sbands[INDEX_80211_BAND_2GHZ].n_channels = domain_2ghz_channels_size;
       sc->sbands[INDEX_80211_BAND_2GHZ].bitrates = ssv6200_legacy_rates;
       sc->sbands[INDEX_80211_BAND_2GHZ].n_bitrates =
        ARRAY_SIZE(ssv6200_legacy_rates);
 }
#else



 if (sc->sh->cfg.hw_caps & SSV6200_HW_CAP_2GHZ) {
        channels = kmemdup(ssv6200_2ghz_chantable,
   sizeof(ssv6200_2ghz_chantable), GFP_KERNEL);
  if (!channels) {
   goto err_create_channel_list;
        }
  sc->sbands[INDEX_80211_BAND_2GHZ].channels = channels;
  sc->sbands[INDEX_80211_BAND_2GHZ].band = INDEX_80211_BAND_2GHZ;
  sc->sbands[INDEX_80211_BAND_2GHZ].n_channels =
   ARRAY_SIZE(ssv6200_2ghz_chantable);
  sc->sbands[INDEX_80211_BAND_2GHZ].bitrates = ssv6200_legacy_rates;
  sc->sbands[INDEX_80211_BAND_2GHZ].n_bitrates =
   ARRAY_SIZE(ssv6200_legacy_rates);
 }
#endif

#ifdef CONFIG_SSV_CUSTOM_DOMAIN
 if (sc->sh->cfg.hw_caps & SSV6200_HW_CAP_5GHZ) {
       u32 domain_5ghz_channels_size = 0;
       channels = kmalloc(sizeof(ssv6200_5ghz_chantable), GFP_KERNEL);
    if (!channels) {
     goto err_create_channel_list;
          }
     switch (sc->sh->cfg.domain)
     {
        case DOMAIN_FCC: 
        case DOMAIN_North_America: 
		case DOMAIN_Taiwan:
           domain_5ghz_channels_size = 25;
           memcpy(channels, &(ssv6200_5ghz_chantable[0]), sizeof(struct ieee80211_channel) * domain_5ghz_channels_size);
     	   break;   
        case DOMAIN_Singapore:    
        case DOMAIN_china: 
		case DOMAIN_ETSI:
           domain_5ghz_channels_size = 13;
           memcpy(channels, &(ssv6200_5ghz_chantable_no_midband[0]), sizeof(struct ieee80211_channel) * domain_5ghz_channels_size);
           break;
        default:
           domain_5ghz_channels_size = 25;
           memcpy(channels, &(ssv6200_5ghz_chantable[0]), sizeof(struct ieee80211_channel) * domain_5ghz_channels_size);
           break;
        }
           
       sc->sbands[INDEX_80211_BAND_5GHZ].channels = channels;
       sc->sbands[INDEX_80211_BAND_5GHZ].band = INDEX_80211_BAND_2GHZ;
       sc->sbands[INDEX_80211_BAND_5GHZ].n_channels =
        domain_5ghz_channels_size;
       sc->sbands[INDEX_80211_BAND_5GHZ].bitrates = ssv6200_legacy_rates + 4;
       sc->sbands[INDEX_80211_BAND_5GHZ].n_bitrates =
        ARRAY_SIZE(ssv6200_legacy_rates) - 4;
 }
#else


 if (sc->sh->cfg.hw_caps & SSV6200_HW_CAP_5GHZ) {
        channels = kmemdup(ssv6200_5ghz_chantable,
   sizeof(ssv6200_5ghz_chantable), GFP_KERNEL);
  if (!channels) {
   goto err_create_channel_list;
        }
  sc->sbands[INDEX_80211_BAND_5GHZ].channels = channels;
  sc->sbands[INDEX_80211_BAND_5GHZ].band = INDEX_80211_BAND_5GHZ;
  sc->sbands[INDEX_80211_BAND_5GHZ].n_channels =
   ARRAY_SIZE(ssv6200_5ghz_chantable);
  sc->sbands[INDEX_80211_BAND_5GHZ].bitrates = ssv6200_legacy_rates + 4;
  sc->sbands[INDEX_80211_BAND_5GHZ].n_bitrates =
   ARRAY_SIZE(ssv6200_legacy_rates) - 4 ;
 }

#endif
 sc->cur_channel = NULL;
 sc->hw_chan = (-1);
 skb_queue_head_init(&sc->rc_report_queue);
 INIT_WORK(&sc->rc_report_work, ssv6xxx_rc_report_work);
 sc->rc_report_workqueue = create_workqueue("ssv6xxx_rc_report");
 sc->rc_report_sechedule = 0;
#ifdef MULTI_THREAD_ENCRYPT
    if (SSV_NEED_SW_CIPHER(sc->sh)){
        skb_queue_head_init(&sc->preprocess_q);
        skb_queue_head_init(&sc->crypted_q);
        #ifdef CONFIG_SSV6XXX_DEBUGFS
        sc->max_preprocess_q_len = 0;
        sc->max_crypted_q_len = 0;
        #endif
        spin_lock_init(&sc->crypt_st_lock);
        INIT_LIST_HEAD(&sc->encrypt_task_head);
        for_each_cpu(cpu, cpu_present_mask)
        {
            struct ssv_encrypt_task_list *ta = kzalloc(sizeof(*ta), GFP_KERNEL);
      memset(ta, 0, sizeof(*ta));
            ta->encrypt_task = kthread_create_on_node(ssv6xxx_encrypt_task, sc, cpu_to_node(cpu), "%d/ssv6xxx_encrypt_task", cpu);
            init_waitqueue_head(&ta->encrypt_wait_q);
            if (!IS_ERR(ta->encrypt_task))
            {
                printk("[MT-ENCRYPT]: create kthread %p for CPU %d, ret = %d\n", ta->encrypt_task, cpu, ret);
    #ifdef KTHREAD_BIND
                kthread_bind(ta->encrypt_task, cpu);
    #endif
                list_add_tail(&ta->list, &sc->encrypt_task_head);
                if (cpu_online(cpu))
                {
                 wake_up_process(ta->encrypt_task);
                    ta->started = 1;
                }
                ta->cpu_no = cpu;
            }
            else
            {
                printk("[MT-ENCRYPT]: Fail to create kthread\n");
            }
        }
        sc->cpu_nfb.notifier_call = ssv6xxx_cpu_callback;
    #ifdef KTHREAD_BIND
        register_cpu_notifier(&sc->cpu_nfb);
    #endif
    }
#endif
    init_waitqueue_head(&sc->tx_wait_q);
    sc->tx_wait_q_woken = 0;
    skb_queue_head_init(&sc->tx_skb_q);
    #ifdef CONFIG_SSV6XXX_DEBUGFS
    sc->max_tx_skb_q_len = 0;
    #endif
    sc->tx_task = kthread_run(ssv6xxx_tx_task, sc, "ssv6xxx_tx_task");
    sc->tx_q_empty = false;
    skb_queue_head_init(&sc->tx_done_q);
    init_waitqueue_head(&sc->rx_wait_q);
    sc->rx_wait_q_woken = 0;
    skb_queue_head_init(&sc->rx_skb_q);
    sc->rx_task = kthread_run(ssv6xxx_rx_task, sc, "ssv6xxx_rx_task");
    if (SSV_NEED_SW_CIPHER(sc->sh)){
        ssv6xxx_preload_sw_cipher();
    }
    init_waitqueue_head(&sc->fw_wait_q);
 sc->log_ctrl = LOG_HWIF;
 sc->sh->priv->dbg_control = true;
 sc->cmd_data.log_to_ram = false;
 sc->cmd_data.dbg_log.size = 0;
 sc->cmd_data.dbg_log.totalsize = 0;
 sc->cmd_data.dbg_log.data = NULL;
    init_timer(&sc->house_keeping);
    sc->house_keeping.expires = jiffies + msecs_to_jiffies(HOUSE_KEEPING_TIMEOUT);
    sc->house_keeping.function = ssv6xxx_house_keeping;
    sc->house_keeping.data = (unsigned long)sc;
    sc->house_keeping_wq= create_singlethread_workqueue("ssv6xxx_house_keeping_wq");
    INIT_WORK(&sc->rx_stuck_work, ssv6xxx_rx_stuck_process);
    INIT_WORK(&sc->mib_edca_work, ssv6xxx_mib_edca_process);
    INIT_WORK(&sc->tx_poll_work, ssv6xxx_tx_poll_process);
#ifdef CONFIG_SSV_CCI_IMPROVEMENT
    INIT_WORK(&sc->cci_clean_work, ssv6xxx_cci_clean_process);
    INIT_WORK(&sc->cci_set_work, ssv6xxx_cci_set_process);
#endif
    INIT_WORK(&sc->set_txpwr_work, ssv6xxx_set_txpwr_process);
    INIT_WORK(&sc->thermal_monitor_work, ssv6xxx_thermal_monitor_process);
    sc->sc_flags |= SC_OP_DIRECTLY_ACK;
    atomic_set(&sc->ampdu_tx_frame, 0);
    sc->directly_ack_high_threshold = sc->sh->cfg.directly_ack_high_threshold;
    sc->directly_ack_low_threshold = sc->sh->cfg.directly_ack_low_threshold;
    sc->force_disable_directly_ack_tx = false;
    return ret;
err_create_channel_list:
 kfree(sc->rx.rx_buf);
 return -ENOMEM;
}
static int ssv6xxx_deinit_softc(struct ssv_softc *sc)
{
    void *channels;
 struct sk_buff* skb;
    u8 remain_size;
    printk("%s():\n", __FUNCTION__);
    if (sc->sh->cfg.hw_caps & SSV6200_HW_CAP_2GHZ) {
        channels = sc->sbands[INDEX_80211_BAND_2GHZ].channels;
        kfree(channels);
    }
    if (sc->sh->cfg.hw_caps & SSV6200_HW_CAP_5GHZ) {
        channels = sc->sbands[INDEX_80211_BAND_5GHZ].channels;
        kfree(channels);
    }
#ifdef CONFIG_SSV_SUPPORT_ANDROID
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&sc->early_suspend);
#endif
    ssv_wakelock_destroy(sc);
#endif
    ssv_skb_free(sc, sc->rx.rx_buf);
    sc->rx.rx_buf = NULL;
 do{
  skb = ssv6200_bcast_dequeue(&sc->bcast_txq, &remain_size);
  if(skb)
            ssv6xxx_txbuf_free_skb(skb, (void*)sc);
  else
   break;
 }while(remain_size);
#ifdef MULTI_THREAD_ENCRYPT
    if (SSV_NEED_SW_CIPHER(sc->sh)){
        struct ssv_encrypt_task_list *qtask = NULL;
        if (!list_empty(&sc->encrypt_task_head)){
            for (qtask = list_entry((&sc->encrypt_task_head)->next, typeof(*qtask), list);
                    !list_empty(&sc->encrypt_task_head);
                    qtask = list_entry((&sc->encrypt_task_head)->next, typeof(*qtask), list))
            {
                list_del(&qtask->list);
                kfree(qtask);
            }
        }
    }
    while((skb = skb_dequeue(&sc->preprocess_q)) != NULL)
    {
        SKB_info *skb_info = (SKB_info *)skb->head;
        if(skb_info->crypt_st == PKT_CRYPT_ST_ENC_PRE)
            ssv6xxx_txbuf_free_skb(skb, (void*)sc);
        else
            dev_kfree_skb_any(skb);
    }
    while((skb = skb_dequeue(&sc->crypted_q)) != NULL)
    {
        SKB_info *skb_info = (SKB_info *)skb->head;
        if(skb_info->crypt_st == PKT_CRYPT_ST_ENC_DONE)
            ssv6xxx_txbuf_free_skb(skb, (void*)sc);
        else
            dev_kfree_skb_any(skb);
    }
    printk("[MT-ENCRYPT]: end of de-init\n");
#endif
    printk("%s(): Clean RX queues.\n", __func__);
    while ((skb = skb_dequeue(&sc->rx_skb_q)) != NULL) {
        dev_kfree_skb_any(skb);
    }
 if (sc->cmd_data.dbg_log.data)
  kfree(sc->cmd_data.dbg_log.data);
 flush_workqueue(sc->rc_report_workqueue);
 destroy_workqueue(sc->rc_report_workqueue);
    destroy_workqueue(sc->config_wq);
    del_timer_sync(&sc->house_keeping);
    destroy_workqueue(sc->house_keeping_wq);
    return 0;
}
static void ssv6xxx_deinit_hwsh(struct ssv_softc *sc)
{
    struct ssv_hw *sh = sc->sh;
    struct list_head *pos, *q;
    struct ssv_hw_cfg *entry;
    mutex_lock(&sh->hw_cfg_mutex);
    list_for_each_safe(pos, q, &sh->hw_cfg) {
        entry = list_entry(pos, struct ssv_hw_cfg, list);
        list_del(pos);
        kfree(entry);
    }
    mutex_unlock(&sh->hw_cfg_mutex);
    if (sh->page_count)
        kfree(sh->page_count);
    kfree(sc->sh);
}
#ifndef SSV_SUPPORT_HAL
void ssv6xxx_hw_set_replay_ignore(struct ssv_hw *sh,u8 ignore)
{
    u32 temp;
    SMAC_REG_READ(sh,ADR_SCRT_SET,&temp);
    temp = temp & SCRT_RPLY_IGNORE_I_MSK;
    temp |= (ignore << SCRT_RPLY_IGNORE_SFT);
    SMAC_REG_WRITE(sh,ADR_SCRT_SET, temp);
}
void ssv6xxx_flash_read_all_map(struct ssv_hw *sh)
{
    return;
}
#endif
extern char *tu_cfgfirmwarepath ;
int ssv6xxx_load_firmware(struct ssv_hw *sh)
{
    int ret=0;
    u8 firmware_name[SSV_FIRMWARE_MAX] = "";
    u8 temp_path[SSV_FIRMWARE_PATH_MAX] = "";
    if (sh->cfg.external_firmware_name[0] != 0x00)
    {
        printk(KERN_INFO "Forced to use firmware \"%s\".\n",
               sh->cfg.external_firmware_name);
        strncpy(firmware_name, sh->cfg.external_firmware_name,
                SSV_FIRMWARE_MAX-1);
    }
    else
    {
        SSV_GET_FW_NAME(sh, firmware_name);
        printk(KERN_INFO "Using firmware \"%s\".\n", firmware_name);
    }
    if (firmware_name[0] == 0x00)
    {
        printk(KERN_INFO "Not match correct CHIP identity\n");
        return -1;
    }
    if (tu_cfgfirmwarepath != NULL)
    {
        snprintf(temp_path, SSV_FIRMWARE_PATH_MAX, "%s%s", tu_cfgfirmwarepath,
                 firmware_name);
        ret = SMAC_LOAD_FW(sh,temp_path, 1);
        printk(KERN_INFO "Using firmware at %s\n", temp_path);
    }
    else if (sh->cfg.firmware_path[0] != 0x00)
    {
        snprintf(temp_path, SSV_FIRMWARE_PATH_MAX, "%s%s",
                 sh->cfg.firmware_path, firmware_name);
        ret = SMAC_LOAD_FW(sh,temp_path, 1);
        printk(KERN_INFO "Using firmware at %s\n", temp_path);
    }
    else
    {
        ret = SMAC_LOAD_FW(sh,firmware_name, 0);
    }
    return ret;
}
#ifdef SSV_SUPPORT_HAL
int tu_ssv6xxx_init_mac(struct ssv_hw *sh)
{
    struct ssv_softc *sc=sh->sc;
    int ret=0;
    u32 regval;
    printk(KERN_INFO "SVN version %d\n", ssv_root_version);
    printk(KERN_INFO "SVN ROOT URL %s \n", SSV_ROOT_URl);
    printk(KERN_INFO "COMPILER HOST %s \n", COMPILERHOST);
    printk(KERN_INFO "COMPILER DATE %s \n", COMPILERDATE);
    printk(KERN_INFO "COMPILER OS %s \n", COMPILEROS);
    printk(KERN_INFO "COMPILER OS ARCH %s \n", COMPILEROSARCH);
    if(sc->ps_status == PWRSV_ENABLE){
#ifdef CONFIG_SSV_SUPPORT_ANDROID
        printk(KERN_INFO "%s: wifi Alive lock timeout after 3 secs!\n",__FUNCTION__);
        {
            ssv_wake_timeout(sc, 3);
            printk(KERN_INFO "wifi Alive lock!\n");
        }
#endif
#ifdef CONFIG_SSV_HW_ENCRYPT_SW_DECRYPT
        HAL_SET_RX_FLOW(sh, RX_DATA_FLOW, RX_HCI);
#else
        HAL_SET_RX_FLOW(sh, RX_DATA_FLOW, RX_CIPHER_HCI);
#endif
        HAL_SET_RX_FLOW(sh, RX_MGMT_FLOW, RX_HCI);
        SSV_SET_RX_CTRL_FLOW(sh);
        HAL_UPDATE_DECISION_TABLE_6(sc->sh, sc->mac_deci_tbl[6]);
        return ret;
    }
    SSV_PHY_ENABLE(sh, false);
    ret = HAL_INIT_MAC(sh);
    if (ret)
        goto exit;
    ret = ssv6xxx_load_firmware(sh);
    if (ret)
        goto exit;
    SSV_PLL_CHK(sh);
    SSV_PHY_ENABLE(sh, true);
    HAL_GET_FW_VERSION(sh, &regval);
    if (regval == FIRWARE_NOT_MATCH_CODE){
        printk(KERN_INFO "Firmware check CHIP ID fail[0x%08x]!!\n",regval);
        ret = -1;
        goto exit;
    }
    else
    {
        if (regval == ssv_firmware_version)
        {
            printk(KERN_INFO "Firmware version %d\n", regval);
        }
        else
        {
            if (sh->cfg.ignore_firmware_version == 0)
            {
                printk(KERN_INFO "Firmware version mapping not match[0x%08x]!!\n",regval);
                printk(KERN_INFO "It's should be [0x%08x]!!\n",ssv_firmware_version);
                ret = -1;
                goto exit;
            }
            else
                printk(KERN_INFO "Force ignore_firmware_version\n");
        }
    }
exit:
    return ret;
}
#else
#define SMAC_REG_READ_CHECK(SH,REG,VAL,FAIL_EXIT) \
    do { \
        ret = SMAC_REG_READ(SH, REG, VAL); \
        if (ret != 0) {\
            printk(KERN_ERR "SMAC_REG_READ Failed @%d.\n", __LINE__); \
            goto exit; \
        } \
    } while (0)
#define SMAC_REG_WRITE_CHECK(SH,REG,VAL,FAIL_EXIT) \
    do { \
        ret = SMAC_REG_WRITE(SH, REG, VAL); \
        if (ret != 0) {\
            printk(KERN_ERR "SMAC_REG_WRITE Failed @%d.\n", __LINE__); \
            goto exit; \
        } \
    } while (0)
#define SMAC_REG_SET_BITS_CHECK(SH,REG,BITS,MASK,FAIL_EXIT) \
    do { \
        ret = SMAC_REG_SET_BITS(SH, REG, BITS, MASK); \
        if (ret != 0) {\
            printk(KERN_ERR "SMAC_REG_SET_BITS Failed @%d.\n", __LINE__); \
            goto exit; \
        } \
    } while (0)
#define SMAC_REG_CONFIRM_CHECK(SH,REG,VAL,FAIL_EXIT) \
    do { \
        ret = SMAC_REG_CONFIRM(SH, REG, VAL); \
        if (ret != 0) {\
            printk(KERN_ERR "SMAC_REG_CONFIRM Failed @%d.\n", __LINE__); \
            goto exit; \
        } \
    } while (0)
int tu_ssv6xxx_init_mac(struct ssv_hw *sh)
{
    struct ssv_softc *sc=sh->sc;
    int i = 0 , ret = 0;
#ifdef SSV6200_ECO
    u32 *ptr, id_len, regval, temp[0x8];
#else
 u32 *ptr, id_len, regval;
#endif
 char *chip_id = sh->chip_id;
    printk(KERN_INFO "SVN version %d\n", ssv_root_version);
    printk(KERN_INFO "SVN ROOT URL %s \n", SSV_ROOT_URl);
    printk(KERN_INFO "COMPILER HOST %s \n", COMPILERHOST);
    printk(KERN_INFO "COMPILER DATE %s \n", COMPILERDATE);
    printk(KERN_INFO "COMPILER OS %s \n", COMPILEROS);
    printk(KERN_INFO "COMPILER OS ARCH %s \n", COMPILEROSARCH);
    SMAC_REG_READ_CHECK(sh, ADR_IC_TIME_TAG_1, &regval, exit);
    sh->chip_tag = ((u64)regval<<32);
    SMAC_REG_READ_CHECK(sh, ADR_IC_TIME_TAG_0, &regval, exit);
    sh->chip_tag |= (regval);
    printk(KERN_INFO "CHIP TAG: %llx \n", sh->chip_tag);
    SMAC_REG_READ_CHECK(sh, ADR_CHIP_ID_3, &regval, exit);
    *((u32 *)&chip_id[0]) = __be32_to_cpu(regval);
    SMAC_REG_READ_CHECK(sh, ADR_CHIP_ID_2, &regval, exit);
    *((u32 *)&chip_id[4]) = __be32_to_cpu(regval);
    SMAC_REG_READ_CHECK(sh, ADR_CHIP_ID_1, &regval, exit);
    *((u32 *)&chip_id[8]) = __be32_to_cpu(regval);
    SMAC_REG_READ_CHECK(sh, ADR_CHIP_ID_0, &regval, exit);
    *((u32 *)&chip_id[12]) = __be32_to_cpu(regval);
    chip_id[12+sizeof(u32)] = 0;
    printk(KERN_INFO "CHIP ID: %s \n",chip_id);
    if(sc->ps_status == PWRSV_ENABLE){
#ifdef CONFIG_SSV_SUPPORT_ANDROID
        printk(KERN_INFO "%s: wifi Alive lock timeout after 3 secs!\n",__FUNCTION__);
        {
            ssv_wake_timeout(sc, 3);
            printk(KERN_INFO "wifi Alive lock!\n");
        }
#endif
#ifdef CONFIG_SSV_HW_ENCRYPT_SW_DECRYPT
        SMAC_REG_WRITE_CHECK(sh, ADR_RX_FLOW_DATA, M_ENG_MACRX|(M_ENG_HWHCI<<4), exit);
#else
        SMAC_REG_WRITE_CHECK(sh, ADR_RX_FLOW_DATA,
                             M_ENG_MACRX|(M_ENG_ENCRYPT_SEC<<4)|(M_ENG_HWHCI<<8),
                             exit);
#endif
        SMAC_REG_WRITE_CHECK(sc->sh, ADR_RX_FLOW_MNG, M_ENG_MACRX|(M_ENG_HWHCI<<4),
                             exit);
        SMAC_REG_WRITE_CHECK(sh, ADR_RX_FLOW_CTRL,
                             M_ENG_MACRX|(M_ENG_CPU<<4)|(M_ENG_HWHCI<<8), exit);
        SMAC_REG_WRITE_CHECK(sc->sh, ADR_MRX_FLT_TB0+6*4, (sc->mac_deci_tbl[6]), exit);
        return ret;
    }
    SMAC_REG_SET_BITS_CHECK(sh, ADR_PHY_EN_1, (0 << RG_PHY_MD_EN_SFT),
                            RG_PHY_MD_EN_MSK, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_BRG_SW_RST, 1 << MAC_SW_RST_SFT, exit);
    do
    {
        SMAC_REG_READ_CHECK(sh, ADR_BRG_SW_RST, & regval, exit);
        i ++;
        if (i >10000){
            printk("MAC reset fail !!!!\n");
            WARN_ON(1);
            ret = 1;
            goto exit;
        }
    } while (regval != 0);
    if (sh->cfg.rx_burstread)
     sh->rx_mode = RX_BURSTREAD_MODE;
 SMAC_REG_WRITE_CHECK(sc->sh, ADR_TXQ4_MTX_Q_AIFSN, 0xffff2101, exit);
    SMAC_REG_SET_BITS_CHECK(sc->sh, ADR_MTX_BCN_EN_MISC, 0,
                            MTX_HALT_MNG_UNTIL_DTIM_MSK, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_CONTROL, 0x12000006, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_RX_TIME_STAMP_CFG, ((28<<MRX_STP_OFST_SFT)|0x01),
                         exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_HCI_TX_RX_INFO_SIZE,
          ((u32)(TXPB_OFFSET) << TX_PBOFFSET_SFT) |
          ((u32)(sh->tx_desc_len) << TX_INFO_SIZE_SFT) |
          ((u32)(sh->rx_desc_len) << RX_INFO_SIZE_SFT) |
          ((u32)(sh->rx_pinfo_pad) << RX_LAST_PHY_SIZE_SFT ),
          exit
    );
    SMAC_REG_READ_CHECK(sh,ADR_MMU_CTRL, &regval, exit);
 regval |= (0xff<<MMU_SHARE_MCU_SFT);
    SMAC_REG_WRITE_CHECK(sh,ADR_MMU_CTRL, regval, exit);
    SMAC_REG_READ_CHECK(sh,ADR_MRX_WATCH_DOG, &regval, exit);
    regval &= 0xfffffff0;
    SMAC_REG_WRITE_CHECK(sh,ADR_MRX_WATCH_DOG, regval, exit);
    SMAC_REG_READ_CHECK(sh, ADR_TRX_ID_THRESHOLD, &id_len, exit);
    id_len = (id_len&0xffff0000 ) |
             (sh->tx_info.tx_id_threshold<<TX_ID_THOLD_SFT)|
             (sh->rx_info.rx_id_threshold<<RX_ID_THOLD_SFT);
    SMAC_REG_WRITE_CHECK(sh, ADR_TRX_ID_THRESHOLD, id_len, exit);
    SMAC_REG_READ_CHECK(sh, ADR_ID_LEN_THREADSHOLD1, &id_len, exit);
    id_len = (id_len&0x0f )|
             (sh->tx_info.tx_page_threshold<<ID_TX_LEN_THOLD_SFT)|
             (sh->rx_info.rx_page_threshold<<ID_RX_LEN_THOLD_SFT);
    SMAC_REG_WRITE_CHECK(sh, ADR_ID_LEN_THREADSHOLD1, id_len, exit);
#ifdef CONFIG_SSV_CABRIO_MB_DEBUG
 SMAC_REG_READ_CHECK(sh, ADR_MB_DBG_CFG3, &regval, exit);
    regval |= (debug_buffer<<0);
    SMAC_REG_WRITE_CHECK(sh, ADR_MB_DBG_CFG3, regval, exit);
 SMAC_REG_READ_CHECK(sh, ADR_MB_DBG_CFG2, &regval, exit);
    regval |= (DEBUG_SIZE<<16);
    SMAC_REG_WRITE_CHECK(sh, ADR_MB_DBG_CFG2, regval, exit);
    SMAC_REG_READ_CHECK(sh, ADR_MB_DBG_CFG1, &regval, exit);
    regval |= (1<<MB_DBG_EN_SFT);
    SMAC_REG_WRITE_CHECK(sh, ADR_MB_DBG_CFG1, regval, exit);
    SMAC_REG_READ_CHECK(sh, ADR_MBOX_HALT_CFG, &regval, exit);
    regval |= (1<<MB_ERR_AUTO_HALT_EN_SFT);
    SMAC_REG_WRITE_CHECK(sh, ADR_MBOX_HALT_CFG, regval, exit);
#endif
SMAC_REG_READ_CHECK(sc->sh, ADR_MTX_BCN_EN_MISC, &regval, exit);
regval|=(1<<MTX_TSF_TIMER_EN_SFT);
SMAC_REG_WRITE_CHECK(sc->sh, ADR_MTX_BCN_EN_MISC, regval, exit);
#ifdef SSV6200_ECO
    SMAC_REG_WRITE_CHECK(sh, 0xcd010004, 0x1213, exit);
    for(i=0;i<SSV_RC_MAX_STA;i++)
    {
        if(i==0)
        {
            sh->hw_buf_ptr[i] = ssv6xxx_pbuf_alloc(sc, sizeof(phy_info_tbl)+
                                                    sizeof(struct ssv6xxx_hw_sec), NOTYPE_BUF);
         if((sh->hw_buf_ptr[i]>>28) != 8)
         {
          printk("opps allocate pbuf error\n");
          WARN_ON(1);
          ret = 1;
          goto exit;
         }
        }
        else
        {
            sh->hw_buf_ptr[i] = ssv6xxx_pbuf_alloc(sc, sizeof(struct ssv6xxx_hw_sec), NOTYPE_BUF);
            if((sh->hw_buf_ptr[i]>>28) != 8)
            {
                printk("opps allocate pbuf error\n");
                WARN_ON(1);
                ret = 1;
                goto exit;
            }
        }
    }
    for (i = 0; i < 0x8; i++) {
        temp[i] = 0;
        temp[i] = ssv6xxx_pbuf_alloc(sc, 256,NOTYPE_BUF);
    }
    for (i = 0; i < 0x8; i++) {
        if(temp[i] == 0x800e0000)
            printk("0x800e0000\n");
        else
            ssv6xxx_pbuf_free(sc,temp[i]);
    }
#else
    sh->hw_buf_ptr = ssv6xxx_pbuf_alloc(sc, sizeof(phy_info_tbl)+
                                            sizeof(struct ssv6xxx_hw_sec), NOTYPE_BUF);
 if((sh->hw_buf_ptr>>28) != 8)
 {
  printk("opps allocate pbuf error\n");
  WARN_ON(1);
  ret = 1;
  goto exit;
 }
#endif
#ifdef SSV6200_ECO
    for(i=0;i<SSV_RC_MAX_STA;i++)
        sh->hw_sec_key[i] = sh->hw_buf_ptr[i];
    for(i=0;i<SSV_RC_MAX_STA;i++)
    {
  int x;
        for(x=0; x<sizeof(struct ssv6xxx_hw_sec); x+=4) {
            SMAC_REG_WRITE_CHECK(sh, sh->hw_sec_key[i]+x, 0, exit);
        }
    }
    SMAC_REG_READ_CHECK(sh, ADR_SCRT_SET, &regval, exit);
 regval &= SCRT_PKT_ID_I_MSK;
 regval |= ((sh->hw_sec_key[0] >> 16) << SCRT_PKT_ID_SFT);
 SMAC_REG_WRITE_CHECK(sh, ADR_SCRT_SET, regval, exit);
    sh->hw_pinfo = sh->hw_sec_key[0] + sizeof(struct ssv6xxx_hw_sec);
    for(i=0, ptr=phy_info_tbl; i<PHY_INFO_TBL1_SIZE; i++, ptr++) {
        SMAC_REG_WRITE_CHECK(sh, ADR_INFO0+i*4, *ptr, exit);
        SMAC_REG_CONFIRM(sh, ADR_INFO0+i*4, *ptr);
    }
#else
    sh->hw_sec_key = sh->hw_buf_ptr;
    for(i=0; i<sizeof(struct ssv6xxx_hw_sec); i+=4) {
        SMAC_REG_WRITE_CHECK(sh, sh->hw_sec_key+i, 0, exit);
    }
    SMAC_REG_READ_CHECK(sh, ADR_SCRT_SET, &regval, exit);
 regval &= SCRT_PKT_ID_I_MSK;
 regval |= ((sh->hw_sec_key >> 16) << SCRT_PKT_ID_SFT);
 SMAC_REG_WRITE_CHECK(sh, ADR_SCRT_SET, regval, exit);
    sh->hw_pinfo = sh->hw_sec_key + sizeof(struct ssv6xxx_hw_sec);
    for(i=0, ptr=phy_info_tbl; i<PHY_INFO_TBL1_SIZE; i++, ptr++) {
        SMAC_REG_WRITE_CHECK(sh, ADR_INFO0+i*4, *ptr, exit);
        SMAC_REG_CONFIRM(sh, ADR_INFO0+i*4, *ptr);
    }
#endif
 for(i=0; i<PHY_INFO_TBL2_SIZE; i++, ptr++) {
        SMAC_REG_WRITE_CHECK(sh, sh->hw_pinfo+i*4, *ptr, exit);
        SMAC_REG_CONFIRM(sh, sh->hw_pinfo+i*4, *ptr);
    }
    for(i=0; i<PHY_INFO_TBL3_SIZE; i++, ptr++) {
        SMAC_REG_WRITE_CHECK(sh, sh->hw_pinfo+(PHY_INFO_TBL2_SIZE<<2)+i*4,
                             *ptr, exit);
        SMAC_REG_CONFIRM(sh, sh->hw_pinfo+(PHY_INFO_TBL2_SIZE<<2)+i*4, *ptr);
    }
    SMAC_REG_WRITE_CHECK(sh, ADR_INFO_RATE_OFFSET, 0x00040000, exit);
 SMAC_REG_WRITE_CHECK(sh, ADR_INFO_IDX_ADDR, sh->hw_pinfo, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_INFO_LEN_ADDR, sh->hw_pinfo+(PHY_INFO_TBL2_SIZE)*4,
                         exit);
 printk("ADR_INFO_IDX_ADDR[%08x] ADR_INFO_LEN_ADDR[%08x]\n", sh->hw_pinfo, sh->hw_pinfo+(PHY_INFO_TBL2_SIZE)*4);
    SMAC_REG_WRITE_CHECK(sh, ADR_GLBLE_SET,
          (0 << OP_MODE_SFT) |
          (0 << SNIFFER_MODE_SFT) |
          (1 << DUP_FLT_SFT) |
          (SSV6200_TX_PKT_RSVD_SETTING << TX_PKT_RSVD_SFT) |
          ((u32)(RXPB_OFFSET) << PB_OFFSET_SFT),
          exit
    );
    SMAC_REG_WRITE_CHECK(sh, ADR_STA_MAC_0, *((u32 *)&sh->cfg.maddr[0][0]), exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_STA_MAC_1, *((u32 *)&sh->cfg.maddr[0][4]), exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_BSSID_0, *((u32 *)&sc->bssid[0]), exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_BSSID_1, *((u32 *)&sc->bssid[4]), exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_TX_ETHER_TYPE_0, 0x00000000, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_TX_ETHER_TYPE_1, 0x00000000, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_RX_ETHER_TYPE_0, 0x00000000, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_RX_ETHER_TYPE_1, 0x00000000, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_REASON_TRAP0, 0x7FBC7F87, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_REASON_TRAP1, 0x0000003F, exit);
#ifndef FW_WSID_WATCH_LIST
    SMAC_REG_WRITE_CHECK(sh, ADR_TRAP_HW_ID, M_ENG_HWHCI, exit);
#else
    SMAC_REG_WRITE_CHECK(sh, ADR_TRAP_HW_ID, M_ENG_CPU, exit);
#endif
    SMAC_REG_WRITE_CHECK(sh, ADR_WSID0, 0x00000000, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_WSID1, 0x00000000, exit);
#ifdef CONFIG_SSV_HW_ENCRYPT_SW_DECRYPT
    SMAC_REG_WRITE_CHECK(sh, ADR_RX_FLOW_DATA, M_ENG_MACRX|(M_ENG_HWHCI<<4), exit);
#else
    SMAC_REG_WRITE_CHECK(sh, ADR_RX_FLOW_DATA,
                         M_ENG_MACRX|(M_ENG_ENCRYPT_SEC<<4)|(M_ENG_HWHCI<<8), exit);
#endif
#if defined(CONFIG_P2P_NOA) || defined(CONFIG_RX_MGMT_CHECK)
    SMAC_REG_WRITE_CHECK(sh, ADR_RX_FLOW_MNG,
                         M_ENG_MACRX|(M_ENG_CPU<<4)|(M_ENG_HWHCI<<8), exit);
#else
    SMAC_REG_WRITE_CHECK(sh, ADR_RX_FLOW_MNG, M_ENG_MACRX|(M_ENG_HWHCI<<4), exit);
#endif
    SMAC_REG_WRITE_CHECK(sh, ADR_RX_FLOW_CTRL,
                         M_ENG_MACRX|(M_ENG_CPU<<4)|(M_ENG_HWHCI<<8),
                         exit);
    ssv6xxx_hw_set_replay_ignore(sh, 1);
    ssv6xxx_update_decision_table(sc);
    SMAC_REG_SET_BITS_CHECK(sc->sh, ADR_GLBLE_SET, SSV6XXX_OPMODE_STA,
                            OP_MODE_MSK, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_SDIO_MASK, 0xfffe1fff, exit);
#ifdef CONFIG_SSV_TX_LOWTHRESHOLD
    SMAC_REG_WRITE_CHECK(sh, ADR_TX_LIMIT_INTR,
                         ( 0x80000000
                          | (sh->tx_info.tx_lowthreshold_id_trigger << 16)
                          | sh->tx_info.tx_lowthreshold_page_trigger),
                         exit);
#else
    SMAC_REG_WRITE_CHECK(sh, ADR_MB_THRESHOLD6, 0x80000000, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_MB_THRESHOLD8, 0x04020000, exit);
    SMAC_REG_WRITE_CHECK(sh, ADR_MB_THRESHOLD9, 0x00000404, exit);
#endif
#ifdef CONFIG_SSV_SUPPORT_BTCX
        SMAC_REG_WRITE_CHECK(sh, ADR_BTCX0,COEXIST_EN_MSK|(WIRE_MODE_SZ<<WIRE_MODE_SFT)
                           |WIFI_TX_SW_POL_MSK | BT_SW_POL_MSK, exit);
        SMAC_REG_WRITE_CHECK(sh, ADR_BTCX1,
                             ( SSV6200_BT_PRI_SMP_TIME
                              | (SSV6200_BT_STA_SMP_TIME << BT_STA_SMP_TIME_SFT)
                              | (SSV6200_WLAN_REMAIN_TIME << WLAN_REMAIN_TIME_SFT)),
                             exit);
        SMAC_REG_WRITE_CHECK(sh, ADR_SWITCH_CTL,BT_2WIRE_EN_MSK, exit);
        SMAC_REG_WRITE_CHECK(sh, ADR_PAD7, 1, exit);
        SMAC_REG_WRITE_CHECK(sh, ADR_PAD8, 0, exit);
        SMAC_REG_WRITE_CHECK(sh, ADR_PAD9, 1, exit);
        SMAC_REG_WRITE_CHECK(sh, ADR_PAD25, 1, exit);
        SMAC_REG_WRITE_CHECK(sh, ADR_PAD27, 8, exit);
        SMAC_REG_WRITE_CHECK(sh, ADR_PAD28, 8, exit);
#endif
    ret = ssv6xxx_load_firmware(sh);
    if (ret)
        goto exit;
    SMAC_REG_SET_BITS_CHECK(sh, ADR_PHY_EN_1, (1 << RG_PHY_MD_EN_SFT),
                            RG_PHY_MD_EN_MSK, exit);
    SMAC_REG_READ_CHECK(sh, FW_VERSION_REG, &regval, exit);
    if (regval == FIRWARE_NOT_MATCH_CODE){
        printk(KERN_INFO "Firmware check CHIP ID fail[0x%08x]!!\n",regval);
        SMAC_REG_WRITE_CHECK(sh, FW_VERSION_REG, 0x0, exit);
        SMAC_REG_READ_CHECK(sh, FW_VERSION_REG, &regval, exit);
        printk(KERN_INFO "E-fuse data 0 is[0x%08x]!!\n",regval);
        ret = -1;
        goto exit;
    }
    else
    {
        if (regval == ssv_firmware_version)
        {
            printk(KERN_INFO "Firmware version %d\n", regval);
        }
        else
        {
            if (sh->cfg.ignore_firmware_version == 0)
            {
                printk(KERN_INFO "Firmware version mapping not match[0x%08x]!!\n",regval);
                printk(KERN_INFO "It's should be [0x%08x]!!\n",ssv_firmware_version);
                ret = -1;
                goto exit;
            }
            else
                printk(KERN_INFO "Force ignore_firmware_version\n");
        }
    }
exit:
    return ret;
}
#endif
void ssv6xxx_deinit_mac(struct ssv_softc *sc)
{
#ifdef SSV6200_ECO
    int i;
    for(i = 0; i < SSV_RC_MAX_STA; i++){
        if(sc->sh->hw_buf_ptr[i]){
            SSV_FREE_PBUF(sc, sc->sh->hw_buf_ptr[i]);
        }
    }
#else
    if(sc->sh->hw_buf_ptr){
     SSV_FREE_PBUF(sc, sc->sh->hw_buf_ptr[i]);
    }
#endif
}
void inline ssv6xxx_deinit_hw(struct ssv_softc *sc)
{
    printk("%s(): \n", __FUNCTION__);
    ssv6xxx_deinit_mac(sc);
    HAL_DETACH_USB_HCI(sc->sh);
    SSV_SET_ON3_ENABLE(sc->sh, false);
}
#ifdef SSV_SUPPORT_HAL
static int tu_ssv6xxx_init_hw(struct ssv_hw *sh)
{
    int ret = 0;
    ssv_cabrio_reg *rf_tbl, *phy_tbl ;
    SSV_SET_ON3_ENABLE(sh, true);
    HAL_WAIT_USB_ROM_READY(sh);
    sh->tx_desc_len = HAL_GET_TX_DESC_SIZE(sh);
    sh->rx_desc_len = HAL_GET_RX_DESC_SIZE(sh);
    sh->rx_pinfo_pad = 0x04;
 sh->rx_mode = RX_NORMAL_MODE;
 SSV_INIT_TX_CFG(sh);
 SSV_INIT_RX_CFG(sh);
    HAL_INIT_GPIO_CFG(sh);
    SSV_INIT_IQK(sh);
    HAL_CHG_IPD_PHYINFO(sh);
    HAL_INIT_CH_CFG(sh);
    HAL_LOAD_PHY_TABLE(sh, &phy_tbl);
    HAL_LOAD_RF_TABLE(sh, &rf_tbl);
    HAL_UPDATE_CFG_HW_PATCH(sh, rf_tbl, phy_tbl);
    HAL_UPDATE_HW_CONFIG(sh, rf_tbl, phy_tbl);
    if (ret == 0) ret = HAL_SET_PLL_PHY_RF(sh, rf_tbl, phy_tbl);
    if (ret == 0) ret = HAL_CHG_PAD_SETTING(sh);
    if (ret == 0) ret = HAL_CHG_CLK_SRC(sh);
    if (ret == 0) ret = HAL_UPDATE_EFUSE_SETTING(sh);
    HAL_UPDATE_PRODUCT_HW_SETTING(sh);
    {
        struct ieee80211_channel chan;
        memset(&chan, 0 , sizeof( struct ieee80211_channel));
        chan.hw_value = sh->cfg.def_chan;
        if ( ret == 0){
             ret=HAL_SET_CHANNEL(sh->sc, &chan, NL80211_CHAN_HT20);
             sh->sc->hw_chan = chan.hw_value;
             sh->sc->hw_chan_type = NL80211_CHAN_HT20;
        }
    }
    HAL_SET_PHY_MODE(sh, true);
    return ret;
}
#else
static int tu_ssv6xxx_init_hw(struct ssv_hw *sh)
{
    int ret=0,i=0,x=0;
#ifdef CONFIG_SSV_CABRIO_E
    u32 regval;
#endif
    sh->tx_desc_len = SSV6XXX_TX_DESC_LEN;
    sh->rx_desc_len = SSV6XXX_RX_DESC_LEN;
    sh->rx_pinfo_pad = 0x04;
 sh->rx_mode = RX_NORMAL_MODE;
 sh->tx_info.tx_id_threshold = SSV6200_ID_TX_THRESHOLD;
    sh->tx_info.tx_page_threshold = SSV6200_PAGE_TX_THRESHOLD;
 sh->tx_info.tx_lowthreshold_id_trigger = SSV6200_TX_LOWTHRESHOLD_ID_TRIGGER;
 sh->tx_info.tx_lowthreshold_page_trigger = SSV6200_TX_LOWTHRESHOLD_PAGE_TRIGGER;
 sh->tx_info.bk_txq_size = SSV6200_ID_AC_BK_OUT_QUEUE;
 sh->tx_info.be_txq_size = SSV6200_ID_AC_BE_OUT_QUEUE;
 sh->tx_info.vi_txq_size = SSV6200_ID_AC_VI_OUT_QUEUE;
 sh->tx_info.vo_txq_size = SSV6200_ID_AC_VO_OUT_QUEUE;
 sh->tx_info.manage_txq_size = SSV6200_ID_MANAGER_QUEUE;
 sh->tx_page_available = SSV6200_PAGE_TX_THRESHOLD;
    sh->ampdu_divider = SSV6200_AMPDU_DIVIDER;
 sh->rx_info.rx_id_threshold = SSV6200_ID_RX_THRESHOLD;
 sh->rx_info.rx_page_threshold = SSV6200_PAGE_RX_THRESHOLD;
    SSV_INIT_IQK(sh);
 memcpy(&(sh->hci.hci_ctrl->tx_info), &(sh->tx_info), sizeof(struct ssv6xxx_tx_hw_info));
 memcpy(&(sh->hci.hci_ctrl->rx_info), &(sh->rx_info), sizeof(struct ssv6xxx_rx_hw_info));
#ifdef CONFIG_SSV_CABRIO_E
    if (sh->cfg.force_chip_identity)
    {
        printk("Force use external RF setting [%08x]\n",sh->cfg.force_chip_identity);
        sh->cfg.chip_identity = sh->cfg.force_chip_identity;
    }
    if(sh->cfg.chip_identity == SSV6051Z)
    {
        sh->p_ch_cfg = &ch_cfg_z[0];
        sh->ch_cfg_size = sizeof(ch_cfg_z) / sizeof(struct ssv6xxx_ch_cfg);
        memcpy(phy_info_tbl,phy_info_6051z,sizeof(phy_info_6051z));
    }
    else if(sh->cfg.chip_identity == SSV6051P)
    {
        sh->p_ch_cfg = &ch_cfg_p[0];
        sh->ch_cfg_size = sizeof(ch_cfg_p) / sizeof(struct ssv6xxx_ch_cfg);
    }
    switch (sh->cfg.chip_identity) {
        case SSV6051Q_P1:
        case SSV6051Q_P2:
        case SSV6051Q:
            printk("SSV6051Q setting\n");
            for (i=0; i<sizeof(ssv6200_rf_tbl)/sizeof(ssv_cabrio_reg); i++){
                if (ssv6200_rf_tbl[i].address == 0xCE010008)
                    ssv6200_rf_tbl[i].data = 0x008DF61B;
                if (ssv6200_rf_tbl[i].address == 0xCE010014)
                    ssv6200_rf_tbl[i].data = 0x3D3E84FE;
                if (ssv6200_rf_tbl[i].address == 0xCE010018)
                    ssv6200_rf_tbl[i].data = 0x01457D79;
                if (ssv6200_rf_tbl[i].address == 0xCE01001C)
                    ssv6200_rf_tbl[i].data = 0x000103A7;
                if (ssv6200_rf_tbl[i].address == 0xCE010020)
                    ssv6200_rf_tbl[i].data = 0x000103A6;
                if (ssv6200_rf_tbl[i].address == 0xCE01002C)
                    ssv6200_rf_tbl[i].data = 0x00032CA8;
                if (ssv6200_rf_tbl[i].address == 0xCE010048)
                    ssv6200_rf_tbl[i].data = 0xFCCCCF27;
                if (ssv6200_rf_tbl[i].address == 0xCE010050)
                    ssv6200_rf_tbl[i].data = 0x0047C000;
            }
            break;
        case SSV6051Z:
            printk("SSV6051Z setting\n");
            for (i=0; i<sizeof(ssv6200_rf_tbl)/sizeof(ssv_cabrio_reg); i++){
                if (ssv6200_rf_tbl[i].address == 0xCE010008)
                    ssv6200_rf_tbl[i].data = 0x004D561C;
                if (ssv6200_rf_tbl[i].address == 0xCE010014)
                    ssv6200_rf_tbl[i].data = 0x3D9E84FE;
                if (ssv6200_rf_tbl[i].address == 0xCE010018)
                    ssv6200_rf_tbl[i].data = 0x00457D79;
                if (ssv6200_rf_tbl[i].address == 0xCE01001C)
                    ssv6200_rf_tbl[i].data = 0x000103EB;
                if (ssv6200_rf_tbl[i].address == 0xCE010020)
                    ssv6200_rf_tbl[i].data = 0x000103EA;
                if (ssv6200_rf_tbl[i].address == 0xCE01002C)
                    ssv6200_rf_tbl[i].data = 0x00062CA8;
                if (ssv6200_rf_tbl[i].address == 0xCE010048)
                    ssv6200_rf_tbl[i].data = 0xFCCCCF27;
                if (ssv6200_rf_tbl[i].address == 0xCE010050)
                    ssv6200_rf_tbl[i].data = 0x0047C000;
            }
            break;
        case SSV6051P:
            printk("SSV6051P setting\n");
            for (i=0; i<sizeof(ssv6200_rf_tbl)/sizeof(ssv_cabrio_reg); i++){
                if (ssv6200_rf_tbl[i].address == 0xCE010008)
                    ssv6200_rf_tbl[i].data = 0x008B7C1C;
                if (ssv6200_rf_tbl[i].address == 0xCE010014)
                    ssv6200_rf_tbl[i].data = 0x3D7E84FE;
                if (ssv6200_rf_tbl[i].address == 0xCE010018)
                    ssv6200_rf_tbl[i].data = 0x01457D79;
                if (ssv6200_rf_tbl[i].address == 0xCE01001C)
                    ssv6200_rf_tbl[i].data = 0x000103EB;
                if (ssv6200_rf_tbl[i].address == 0xCE010020)
                    ssv6200_rf_tbl[i].data = 0x000103EA;
                if (ssv6200_rf_tbl[i].address == 0xCE01002C)
                    ssv6200_rf_tbl[i].data = 0x00032CA8;
                if (ssv6200_rf_tbl[i].address == 0xCE010048)
                    ssv6200_rf_tbl[i].data = 0xFCCCCC27;
                if (ssv6200_rf_tbl[i].address == 0xCE010050)
                    ssv6200_rf_tbl[i].data = 0x0047C000;
                if (ssv6200_rf_tbl[i].address == 0xC0001D00)
                    ssv6200_rf_tbl[i].data = 0x5F000040;
            }
            break;
        default:
            printk("No RF setting\n");
            printk("**************\n");
            printk("* Call Help! *\n");
            printk("**************\n");
            WARN_ON(1);
            return -1;
            break;
    }
    if(sh->cfg.crystal_type == SSV6XXX_IQK_CFG_XTAL_26M)
    {
        sh->iqk_cfg.cfg_xtal = SSV6XXX_IQK_CFG_XTAL_26M;
        printk("SSV6XXX_IQK_CFG_XTAL_26M\n");
    }
    else if(sh->cfg.crystal_type == SSV6XXX_IQK_CFG_XTAL_40M)
    {
        sh->iqk_cfg.cfg_xtal = SSV6XXX_IQK_CFG_XTAL_40M;
        printk("SSV6XXX_IQK_CFG_XTAL_40M\n");
    }
    else if(sh->cfg.crystal_type == SSV6XXX_IQK_CFG_XTAL_24M)
    {
        printk("SSV6XXX_IQK_CFG_XTAL_24M\n");
        sh->iqk_cfg.cfg_xtal = SSV6XXX_IQK_CFG_XTAL_24M;
        for(i=0; i<sizeof(ssv6200_rf_tbl)/sizeof(ssv_cabrio_reg); i++)
        {
            if(ssv6200_rf_tbl[i].address == ADR_SX_ENABLE_REGISTER)
                ssv6200_rf_tbl[i].data = 0x0003E07C;
            if(ssv6200_rf_tbl[i].address == ADR_DPLL_DIVIDER_REGISTER)
                ssv6200_rf_tbl[i].data = 0x00406000;
            if(ssv6200_rf_tbl[i].address == ADR_DPLL_FB_DIVIDER_REGISTERS_I)
                ssv6200_rf_tbl[i].data = 0x00000028;
            if(ssv6200_rf_tbl[i].address == ADR_DPLL_FB_DIVIDER_REGISTERS_II)
                ssv6200_rf_tbl[i].data = 0x00000000;
        }
    }
    else
    {
        printk("Illegal xtal setting !![No XX.cfg]\n");
        printk("default value is SSV6XXX_IQK_CFG_XTAL_26M!!\n");
    }
    for(i=0; i<sizeof(ssv6200_rf_tbl)/sizeof(ssv_cabrio_reg); i++)
    {
        if(ssv6200_rf_tbl[i].address == ADR_SYN_KVCO_XO_FINE_TUNE_CBANK)
        {
            if(sh->cfg.crystal_frequency_offset)
            {
                ssv6200_rf_tbl[i].data &= RG_XOSC_CBANK_XO_I_MSK;
                ssv6200_rf_tbl[i].data |= (sh->cfg.crystal_frequency_offset << RG_XOSC_CBANK_XO_SFT);
            }
        }
    }
    for (i=0; i<sizeof(phy_setting)/sizeof(ssv_cabrio_reg); i++){
        if (phy_setting[i].address == ADR_TX_GAIN_FACTOR){
            switch (sh->cfg.chip_identity) {
                case SSV6051Q_P1:
                case SSV6051Q_P2:
                case SSV6051Q:
                    printk("SSV6051Q setting [0x5B606C72]\n");
                    phy_setting[i].data = 0x5B606C72;
     break;
                case SSV6051Z:
                    printk("SSV6051Z setting [0x60606060]\n");
                    phy_setting[i].data = 0x60606060;
                    break;
                case SSV6051P:
                    printk("SSV6051P setting [0x6C726C72]\n");
                    phy_setting[i].data = 0x6C726C72;
                    break;
                default:
                    printk("Use default power setting\n");
                    break;
            }
            if (sh->cfg.wifi_tx_gain_level_b){
                phy_setting[i].data &= 0xffff0000;
                phy_setting[i].data |= wifi_tx_gain[sh->cfg.wifi_tx_gain_level_b] & 0x0000ffff;
            }
            if (sh->cfg.wifi_tx_gain_level_gn){
                phy_setting[i].data &= 0x0000ffff;
                phy_setting[i].data |= wifi_tx_gain[sh->cfg.wifi_tx_gain_level_gn] & 0xffff0000;
            }
            printk("TX power setting 0x%x\n",phy_setting[i].data);
            sh->iqk_cfg.cfg_def_tx_scale_11b = (phy_setting[i].data>>0) & 0xff;
            sh->iqk_cfg.cfg_def_tx_scale_11b_p0d5 = (phy_setting[i].data>>8) & 0xff;
            sh->iqk_cfg.cfg_def_tx_scale_11g = (phy_setting[i].data>>16) & 0xff;
            sh->iqk_cfg.cfg_def_tx_scale_11g_p0d5 = (phy_setting[i].data>>24) & 0xff;
            break;
        }
    }
    if(sh->cfg.volt_regulator == SSV6XXX_VOLT_LDO_CONVERT)
    {
        printk("Volt regulator LDO\n");
        for(i=0; i<sizeof(ssv6200_rf_tbl)/sizeof(ssv_cabrio_reg); i++)
        {
            if(ssv6200_rf_tbl[i].address == ADR_PMU_2)
            {
                ssv6200_rf_tbl[i].data &= 0xFFFFFFFE;
                ssv6200_rf_tbl[i].data |= 0x00000000;
            }
        }
    }
    else if(sh->cfg.volt_regulator == SSV6XXX_VOLT_DCDC_CONVERT)
    {
        printk("Volt regulator DCDC\n");
        for(i=0; i<sizeof(ssv6200_rf_tbl)/sizeof(ssv_cabrio_reg); i++)
        {
            if(ssv6200_rf_tbl[i].address == ADR_PMU_2)
            {
                ssv6200_rf_tbl[i].data &= 0xFFFFFFFE;
                ssv6200_rf_tbl[i].data |= 0x00000001;
            }
        }
    }
    else
    {
        printk("Illegal volt regulator setting !![No XX.cfg]\n");
        printk("default value is SSV6XXX_VOLT_DCDC_CONVERT!!\n");
    }
#endif
    while(tu_ssv_cfg.configuration[x][0])
    {
        for(i=0; i<sizeof(ssv6200_rf_tbl)/sizeof(ssv_cabrio_reg); i++)
        {
            if(ssv6200_rf_tbl[i].address == tu_ssv_cfg.configuration[x][0])
            {
                ssv6200_rf_tbl[i].data = tu_ssv_cfg.configuration[x][1];
                break;
            }
        }
        for(i=0; i<sizeof(phy_setting)/sizeof(ssv_cabrio_reg); i++)
        {
            if(phy_setting[i].address == tu_ssv_cfg.configuration[x][0])
            {
                phy_setting[i].data = tu_ssv_cfg.configuration[x][1];
                break;
            }
        }
        x++;
    };
    if (ret == 0) ret = SSV6XXX_SET_HW_TABLE(sh, ssv6200_rf_tbl);
    if (ret == 0) ret = SMAC_REG_WRITE(sh, ADR_PHY_EN_1, 0x00000000);
#ifdef CONFIG_SSV_CABRIO_E
    SMAC_REG_READ(sh, ADR_PHY_EN_0, &regval);
    if (regval & (1<<RG_RF_BB_CLK_SEL_SFT)) {
        printk("already do clock switch\n");
    }
    else {
        printk("reset PLL\n");
        SMAC_REG_READ(sh, ADR_DPLL_CP_PFD_REGISTER, &regval);
        regval |= ((1<<RG_DP_BBPLL_PD_SFT) | (1<<RG_DP_BBPLL_SDM_EDGE_SFT));
        SMAC_REG_WRITE(sh, ADR_DPLL_CP_PFD_REGISTER, regval);
        regval &= ~((1<<RG_DP_BBPLL_PD_SFT) | (1<<RG_DP_BBPLL_SDM_EDGE_SFT));
        SMAC_REG_WRITE(sh, ADR_DPLL_CP_PFD_REGISTER, regval);
        mdelay(10);
    }
#endif
    if (ret == 0) ret = SSV6XXX_SET_HW_TABLE(sh, ssv6200_phy_tbl);
#ifdef CONFIG_SSV_CABRIO_E
    if (ret == 0) ret = SMAC_REG_WRITE(sh, ADR_TRX_DUMMY_REGISTER, 0xEAAAAAAA);
    SMAC_REG_READ(sh,ADR_TRX_DUMMY_REGISTER,&regval);
    if (regval != 0xEAAAAAAA)
    {
        printk("@@@@@@@@@@@@\n");
        printk(" SDIO issue -- please check 0xCE01008C %08x!!\n",regval);
        printk(" It shouble be 0xEAAAAAAA!!\n");
        printk("@@@@@@@@@@@@ \n");
    }
#endif
#ifdef CONFIG_SSV_CABRIO_E
    if (ret == 0) ret = SMAC_REG_WRITE(sh, ADR_PAD53, 0x21);
    if (ret == 0) ret = SMAC_REG_WRITE(sh, ADR_PAD54, 0x3000);
    if (ret == 0) ret = SMAC_REG_WRITE(sh, ADR_PIN_SEL_0, 0x4000);
    if (ret == 0) ret = SMAC_REG_WRITE(sh, 0xc0000304, 0x01);
    if (ret == 0) ret = SMAC_REG_WRITE(sh, 0xc0000308, 0x01);
#endif
    if (ret == 0) ret = SMAC_REG_WRITE(sh, ADR_CLOCK_SELECTION, 0x3);
#ifdef CONFIG_SSV_CABRIO_E
    if (ret == 0) ret = SMAC_REG_WRITE(sh, ADR_TRX_DUMMY_REGISTER, 0xAAAAAAAA);
#endif
    {
        struct ieee80211_channel chan;
        memset(&chan, 0 , sizeof( struct ieee80211_channel));
        chan.hw_value = sh->cfg.def_chan;
        if ((ret=ssv6xxx_set_channel(sh->sc, &chan, NL80211_CHAN_HT20)))
            return ret;
    }
    if (ret == 0) ret = SMAC_REG_WRITE(sh, ADR_PHY_EN_1,
                                (RG_PHYRX_MD_EN_MSK | RG_PHYTX_MD_EN_MSK |
                                RG_PHY11GN_MD_EN_MSK | RG_PHY11B_MD_EN_MSK |
                                RG_PHYRXFIFO_MD_EN_MSK | RG_PHYTXFIFO_MD_EN_MSK |
                                RG_PHY11BGN_MD_EN_MSK));
    return ret;
}
#endif
static void ssv6xxx_save_hw_config(void *param, u32 addr, u32 value)
{
 struct ssv_softc *sc = (struct ssv_softc *)param;
    struct ssv_hw *sh = sc->sh;
    struct list_head *pos, *q;
    struct ssv_hw_cfg *entry;
    struct ssv_hw_cfg *new_cfg;
    bool find = false;
    if (!(sc->sh->cfg.online_reset & ONLINE_RESET_ENABLE) || (sc->sc_flags & SC_OP_HW_RESET))
        return;
    mutex_lock(&sh->hw_cfg_mutex);
    list_for_each_safe(pos, q, &sh->hw_cfg) {
        entry = list_entry(pos, struct ssv_hw_cfg, list);
        if (entry->addr == addr) {
            entry->value = value;
            find = true;
            break;
        }
    }
    if (!find) {
        new_cfg = (struct ssv_hw_cfg *)kmalloc(sizeof(struct ssv_hw_cfg), GFP_KERNEL);
        memset(new_cfg, 0, sizeof(struct ssv_hw_cfg));
        new_cfg->addr = addr;
        new_cfg->value = value;
        list_add_tail(&(new_cfg->list), &(sh->hw_cfg));
    }
    mutex_unlock(&sh->hw_cfg_mutex);
}
static void ssv6xxx_restore_hw_config(struct ssv_softc *sc)
{
    struct ssv_hw *sh = sc->sh;
    struct list_head *pos, *q;
    struct ssv_hw_cfg *entry;
    mutex_lock(&sh->hw_cfg_mutex);
    list_for_each_safe(pos, q, &sh->hw_cfg) {
        entry = list_entry(pos, struct ssv_hw_cfg, list);
        SMAC_REG_WRITE(sh, entry->addr, entry->value);
    }
    mutex_unlock(&sh->hw_cfg_mutex);
}
void ssv6xxx_restart_hw(struct work_struct *work)
{
    struct ssv_softc *sc = container_of(work, struct ssv_softc, hw_restart_work);
    struct ieee80211_channel *chan;
    enum nl80211_channel_type channel_type;
    int i = 0, ret = 0;
    printk("**************************\n");
    printk("*** Software MAC reset ***\n");
    printk("**************************\n");
    sc->sc_flags |= SC_OP_HW_RESET;
    sc->restart_counter++;
    sc->force_triger_reset = true;
    while (sc->bScanning) {
        mdelay(100);
        if (++i > 10000)
            return;
    }
    rtnl_lock();
    mutex_lock(&sc->mutex);
    printk("%s() start\n", __FUNCTION__);
    SSV_BEACON_LOSS_DISABLE(sc->sh);
    HCI_STOP(sc->sh);
    SSV_PHY_ENABLE(sc->sh, false);
    sc->beacon_info[0].pubf_addr = 0x00;
    sc->beacon_info[1].pubf_addr = 0x00;
    SSV_SAVE_HW_STATUS(sc);
    SSV_RESET_SYSPLF(sc->sh);
    udelay(50);
    tu_ssv6xxx_init_hw(sc->sh);
    tu_ssv6xxx_init_mac(sc->sh);
    ssv6xxx_restore_hw_config(sc);
    HCI_START(sc->sh);
    ret = SSV_DO_IQ_CALIB(sc->sh, &sc->sh->iqk_cfg);
    if (ret != 0) {
        printk("IQ Calibration failed (%d)!!\n", ret);
    }
    SSV_EDCA_ENABLE(sc->sh, true);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
    chan = sc->hw->conf.channel;
    channel_type = sc->hw->conf.channel_type;
#else
    chan = sc->hw->conf.chandef.chan;
    channel_type = cfg80211_get_chandef_type(&sc->hw->conf.chandef);
#endif
    HAL_SET_CHANNEL(sc, chan, channel_type);
    SSV_AMPDU_AUTO_CRC_EN(sc->sh);
    SSV_SET_RF_ENABLE(sc->sh);
    if (sc->isAssoc )
        SSV_BEACON_LOSS_ENABLE(sc->sh);
    mutex_unlock(&sc->mutex);
    rtnl_unlock();
    sc->sc_flags &= (~(SC_OP_HW_RESET));
    sc->house_keeping.expires = jiffies + msecs_to_jiffies(HOUSE_KEEPING_TIMEOUT);
 if (!timer_pending(&sc->house_keeping))
        add_timer(&sc->house_keeping);
}
static void ssv6xxx_check_mac2(struct ssv_hw *sh)
{
  const u8 addr_mask[6]={0xfd, 0xff, 0xff, 0xff, 0xff, 0xfc};
  u8 i;
  bool invalid = false;
  for ( i=0; i<6; i++) {
       if ((tu_ssv_cfg.maddr[0][i] & addr_mask[i]) !=
               (tu_ssv_cfg.maddr[1][i] & addr_mask[i])){
           invalid = true;
           printk (" i %d , mac1[i] %x, mac2[i] %x, mask %x \n",i, tu_ssv_cfg.maddr[0][i] ,tu_ssv_cfg.maddr[1][i],addr_mask[i]);
           break;
       }
   }
   if (invalid){
       memcpy(&tu_ssv_cfg.maddr[1][0], &tu_ssv_cfg.maddr[0][0], 6);
       tu_ssv_cfg.maddr[1][5] ^= 0x01;
       if (tu_ssv_cfg.maddr[1][5] < tu_ssv_cfg.maddr[0][5]){
           u8 temp;
           temp = tu_ssv_cfg.maddr[0][5];
           tu_ssv_cfg.maddr[0][5] = tu_ssv_cfg.maddr[1][5];
           tu_ssv_cfg.maddr[1][5] = temp;
           sh->cfg.maddr[0][5] = tu_ssv_cfg.maddr[0][5];
       }
       printk("MAC 2 address invalid!!\n" );
       printk("After modification, MAC1 %pM, MAC2 %pM\n",tu_ssv_cfg.maddr[0],
           tu_ssv_cfg.maddr[1]);
   }
}
static int ssv6xxx_read_configuration(struct ssv_hw *sh)
{
    memcpy(&sh->cfg, &tu_ssv_cfg, sizeof(struct ssv6xxx_cfg));
    efuse_read_all_map(sh);
    SSV_FLASH_READ_ALL_MAP(sh);
    if (!(is_valid_ether_addr(&sh->cfg.maddr[0][0]))){
        printk("invalid mac addr 1 !!\n");
        WARN_ON(1);
        return 1;
    }
    if (SSV_IF_CHK_MAC2(sh)) {
        ssv6xxx_check_mac2(sh);
        memcpy(&sh->cfg.maddr[1][0], &tu_ssv_cfg.maddr[1][0], ETH_ALEN);
    }
    #if 0
    if(sh->cfg.crystal_type == 26)
        sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_26M;
    else if(sh->cfg.crystal_type == 40)
        sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_40M;
    else if(sh->cfg.crystal_type == 24)
        sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_24M;
    else if(sh->cfg.crystal_type == 25)
        sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_25M;
    else
    {
        printk("Please redefine xtal_clock(wifi.cfg)!!\n");
        WARN_ON(1);
        return 1;
    }
    #endif
    switch (sh->cfg.crystal_type){
        case 16:
            sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_16M;
            break;
        case 24:
            sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_24M;
            break;
        case 26:
            sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_26M;
            break;
        case 40:
            sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_40M;
            break;
        case 12:
            sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_12M;
            break;
        case 20:
            sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_20M;
            break;
        case 25:
            sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_25M;
            break;
        case 32:
            sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_32M;
            break;
        default:
            printk("Please redefine xtal_clock(wifi.cfg)!!\n");
            WARN_ON(1);
            return 1;
            break;
    }
    if (!sh->flash_config.exist) {
        if(sh->cfg.volt_regulator < 2)
            sh->cfg.volt_regulator = tu_ssv_cfg.volt_regulator;
        else
        {
            printk("Please redefine volt_regulator(wifi.cfg)!!\n");
            WARN_ON(1);
            return 1;
        }
    }
    SSV_ADJ_CONFIG(sh);
    return 0;
}
static int ssv6xxx_hci_rx_mode(void *args)
{
 struct ssv_softc *sc = (struct ssv_softc *)args;
    struct ssv_hw *sh = sc->sh;
 return sh->rx_mode;
}
static int ssv6xxx_read_hw_info(struct ssv_softc *sc)
{
    struct ssv_hw *sh = sc->sh;
    SSV_GET_IC_TIME_TAG(sh);
    printk(KERN_INFO "CHIP TAG: %llx \n", sh->chip_tag);
    if (ssv6xxx_read_configuration(sh))
        return -ENOMEM;
    sh->hci.hci_post_tx_cb= ssv6xxx_post_tx_cb;
    sh->hci.hci_pre_tx_cb = ssv6xxx_pre_tx_cb;
    sh->hci.hci_skb_update_cb = ssv6xxx_tx_rate_update;
    sh->hci.hci_tx_flow_ctrl_cb = ssv6200_tx_flow_control;
    sh->hci.hci_tx_q_empty_cb = ssv6xxx_tx_q_empty_cb;
    sh->hci.hci_tx_buf_free_cb = ssv6xxx_txbuf_free_skb;
    sh->hci.hci_rx_mode_cb = ssv6xxx_hci_rx_mode;
    sh->hci.hci_peek_next_pkt_len_cb = ssv6xxx_peek_next_pkt_len;
    sh->hci.skb_alloc = ssv_skb_alloc;
    sh->hci.skb_free = ssv_skb_free;
 sh->hci.dbgprint = ssv6xxx_hci_dbgprint;
    sh->hci.write_hw_config_cb = ssv6xxx_save_hw_config;
    sh->write_hw_config_cb = ssv6xxx_save_hw_config;
    sh->write_hw_config_args = (void *)sc;
    return 0;
}
#ifndef SSV_SUPPORT_HAL
static int _alloc_sh (struct ssv_softc *sc)
{
    struct ssv_hw *sh;
    sh = kzalloc(sizeof(struct ssv_hw), GFP_KERNEL);
    if (sh == NULL)
        return -ENOMEM;
    memset((void *)sh, 0, sizeof(struct ssv_hw));
    sh->page_count = (u8 *)kzalloc(sizeof(u8) * SSV6200_ID_NUMBER, GFP_KERNEL);
    if (sh->page_count == NULL) {
        kfree(sh);
        return -ENOMEM;
    }
    memset(sh->page_count, 0, sizeof(u8) * SSV6200_ID_NUMBER);
    sc->sh = sh;
    sh->sc = sc;
    INIT_LIST_HEAD(&sh->hw_cfg);
    mutex_init(&sh->hw_cfg_mutex);
    sh->priv = sc->dev->platform_data;
    sh->hci.if_ops = sh->priv->ops;
    sh->hci.dev = sc->dev;
    sh->hci.skb_alloc = ssv_skb_alloc;
    sh->hci.skb_free = ssv_skb_free;
    sh->hci.hci_rx_cb = ssv6200_rx;
    sh->hci.hci_is_rx_q_full = ssv6200_is_rx_q_full;
    sh->priv->skb_alloc = ssv_skb_alloc_ex;
    sh->priv->skb_free = ssv_skb_free;
    sh->priv->skb_param = sc;
#ifdef CONFIG_PM
 sh->priv->suspend = ssv6xxx_power_sleep;
 sh->priv->resume = ssv6xxx_power_awake;
 sh->priv->pm_param = sc;
#endif
    sh->priv->enable_usb_acc = ssv6xxx_enable_usb_acc;
    sh->priv->disable_usb_acc = ssv6xxx_disable_usb_acc;
    sh->priv->jump_to_rom = ssv6xxx_jump_to_rom;
    sh->priv->usb_param = sc;
    sh->priv->rx_burstread_size = ssv6xxx_rx_burstread_size;
    sh->priv->rx_burstread_param = sc;
    sh->hci.sc = sc;
    sh->hci.sh = sh;
    return 0;
}
#endif
static int tu_ssv6xxx_init_device(struct ssv_softc *sc, const char *name)
{
#ifndef CONFIG_SSV6XXX_HW_DEBUG
    struct ieee80211_hw *hw = sc->hw;
#endif
    struct ssv_hw *sh;
    int error = 0;
    BUG_ON(!sc->dev->platform_data);
#ifndef SSV_SUPPORT_HAL
    if ((error = _alloc_sh(sc)) != 0) {
#else
    if ((error = tu_ssv6xxx_init_hal(sc)) != 0) {
#endif
        goto err;
    }
    sh = sc->sh;
    if ((error = tu_ssv6xxx_hci_register(&sh->hci)) != 0)
        goto err_sh;;
    if ((error = ssv6xxx_read_hw_info(sc)) != 0) {
        goto err_hci;
    }
    if (sh->cfg.hw_caps == 0) {
        error = -1;
        goto err_hci;
    }
    if ((error = tu_ssv6xxx_init_softc(sc)) != 0) {
        goto err_softc;
    }
    ssv6xxx_set_80211_hw_capab(sc);
    if ((error = tu_ssv6xxx_init_hw(sc->sh)) != 0) {
        goto err_hw;
    }
#ifndef CONFIG_SSV6XXX_HW_DEBUG
    if ((error = ieee80211_register_hw(hw)) != 0) {
        printk(KERN_ERR "Failed to register w. %d.\n", error);
        goto err_hw;
    }
#endif
#ifndef CONFIG_SSV6XXX_HW_DEBUG
    ssv_init_cli(dev_name(&hw->wiphy->dev), &sc->cmd_data);
#else
    ssv_init_cli(dev_name(sc->dev), &sc->cmd_data);
#endif
#ifdef CONFIG_SSV6XXX_DEBUGFS
    tu_ssv6xxx_init_debugfs(sc, name);
#endif
#ifdef CONFIG_SSV_SMARTLINK
    {
        extern int ksmartlink_init(void);
        (void)ksmartlink_init();
    }
#endif
    sc->sc_flags |= SC_OP_DEV_READY;
    return 0;
err_hw:
    ssv6xxx_deinit_hw(sc);
err_softc:
    ssv6xxx_deinit_softc(sc);
err_hci:
    tu_ssv6xxx_hci_deregister(&sh->hci);
err_sh:
    ssv6xxx_deinit_hwsh(sc);
err:
    return error;
}
int ssv6xxx_rate_control_register(void)
{
 int ret = 0;
#if (!defined(SSV_SUPPORT_HAL)||defined(SSV_SUPPORT_SSV6051))
 ret = ssv6xxx_pid_rate_control_register();
 if (ret)
  return ret;
#endif
 ret = ssv6xxx_minstrel_rate_control_register();
 if (ret)
  goto err_ssv_minstrel;
 return 0;
err_ssv_minstrel:
#if (!defined(SSV_SUPPORT_HAL)||defined(SSV_SUPPORT_SSV6051))
 ssv6xxx_pid_rate_control_unregister();
#endif
 return ret;
}
void ssv6xxx_rate_control_unregister(void)
{
#if (!defined(SSV_SUPPORT_HAL)||defined(SSV_SUPPORT_SSV6051))
 ssv6xxx_pid_rate_control_unregister();
#endif
 ssv6xxx_minstrel_rate_control_unregister();
}
static void ssv6xxx_deinit_device(struct ssv_softc *sc)
{
    printk("%s(): \n", __FUNCTION__);
    sc->sc_flags &= ~SC_OP_DEV_READY;
#ifdef CONFIG_SSV_SMARTLINK
    {
        extern void ksmartlink_exit(void);
        ksmartlink_exit();
    }
#endif
#ifdef CONFIG_SSV6XXX_DEBUGFS
    ssv6xxx_deinit_debugfs(sc);
#endif
#ifndef CONFIG_SSV6XXX_HW_DEBUG
    ssv_deinit_cli(dev_name(&sc->hw->wiphy->dev), &sc->cmd_data);
#else
    ssv_deinit_cli(dev_name(sc->dev), &sc->cmd_data);
#endif
    SSV_SET_RF_DISABLE(sc->sh);
#ifndef CONFIG_SSV6XXX_HW_DEBUG
    ieee80211_unregister_hw(sc->hw);
#endif
    ssv6xxx_deinit_hw(sc);
    ssv6xxx_deinit_softc(sc);
    tu_ssv6xxx_hci_deregister(&sc->sh->hci);
    ssv6xxx_deinit_hwsh(sc);
}
extern struct ieee80211_ops ssv6200_ops;
int tu_ssv6xxx_dev_probe(struct platform_device *pdev)
{
    struct ssv_softc *sc;
    struct ieee80211_hw *hw;
    int ret;
    if (!pdev->dev.platform_data) {
        dev_err(&pdev->dev, "no platform data specified!\n");
        return -EINVAL;
    }
    printk("%s(): SSV6X5X device \"%s\" found !\n", __FUNCTION__, pdev->name);
#ifdef SSV_MAC80211
 hw = ieee80211_alloc_hw_nm(sizeof(struct ssv_softc), &ssv6200_ops,"icomm");
#else
 hw = ieee80211_alloc_hw(sizeof(struct ssv_softc), &ssv6200_ops);
#endif
 if (hw == NULL) {
        dev_err(&pdev->dev, "No memory for ieee80211_hw\n");
        return -ENOMEM;
    }
    SET_IEEE80211_DEV(hw, &pdev->dev);
    dev_set_drvdata(&pdev->dev, hw);
    memset((void *)hw->priv, 0, sizeof(struct ssv_softc));
    sc = hw->priv;
    sc->hw = hw;
    sc->dev = &pdev->dev;
    sc->platform_dev = pdev;
    ret = tu_ssv6xxx_init_device(sc, pdev->name);
    if (ret) {
        dev_err(&pdev->dev, "Failed to initialize device\n");
        ieee80211_free_hw(hw);
        return ret;
    }
    wiphy_info(hw->wiphy, "%s\n", "SSV6X5X of iComm-semi");
    return 0;
}
EXPORT_SYMBOL(tu_ssv6xxx_dev_probe);
static void ssv6xxx_stop_all_running_threads(struct ssv_softc *sc) {
    if (sc->ssv_txtput.txtput_tsk) {
        kthread_stop(sc->ssv_txtput.txtput_tsk);
        sc->ssv_txtput.txtput_tsk = NULL;
    }
    cancel_work_sync(&sc->beacon_miss_work);
    cancel_delayed_work_sync(&sc->bcast_tx_work);
    sc->rc_report_sechedule = 0;
    cancel_work_sync(&sc->rc_report_work);
    cancel_work_sync(&sc->rx_stuck_work);
    cancel_work_sync(&sc->mib_edca_work);
    cancel_work_sync(&sc->tx_poll_work);
#ifdef CONFIG_SSV_CCI_IMPROVEMENT
    cancel_work_sync(&sc->cci_clean_work);
    cancel_work_sync(&sc->cci_set_work);
#endif
    cancel_work_sync(&sc->set_txpwr_work);
 cancel_work_sync(&sc->thermal_monitor_work);
#ifdef MULTI_THREAD_ENCRYPT
    if (SSV_NEED_SW_CIPHER(sc->sh)){
        unregister_cpu_notifier(&sc->cpu_nfb);
        if (!list_empty(&sc->encrypt_task_head))
        { int counter = 0;
            struct ssv_encrypt_task_list *qtask = NULL;
            for (qtask = list_entry((&sc->encrypt_task_head)->next, typeof(*qtask), list);
                    !list_empty(&sc->encrypt_task_head);
                    qtask = list_entry((&sc->encrypt_task_head)->next, typeof(*qtask), list))
            {
                counter++;
                printk("Stopping encrypt task %d: ...\n", counter);
                kthread_stop(qtask->encrypt_task);
                printk("encrypt task %d is stopped: ...\n", counter);
            }
        }
    }
#endif
    if (sc->tx_task != NULL)
    {
        printk("Stopping TX task...\n");
        kthread_stop(sc->tx_task);
  sc->tx_task = NULL;
        printk("TX task is stopped.\n");
    }
    if (sc->rx_task != NULL)
    {
        printk("Stopping RX task...\n");
        kthread_stop(sc->rx_task);
  sc->rx_task = NULL;
        printk("RX task is stopped.\n");
    }
    if(sc->sh->hci.hci_ctrl->hci_tx_task != NULL) {
        printk("Stopping HCI TX task...\n");
        kthread_stop(sc->sh->hci.hci_ctrl->hci_tx_task);
        sc->sh->hci.hci_ctrl->hci_tx_task = NULL;
        printk("HCI TX task is stopped.\n");
    }
}
int tu_ssv6xxx_dev_remove(struct platform_device *pdev)
{
    struct ieee80211_hw *hw=dev_get_drvdata(&pdev->dev);
    struct ssv_softc *sc=hw->priv;
    printk("tu_ssv6xxx_dev_remove(): pdev=%p, hw=%p\n", pdev, hw);
    ssv6xxx_stop_all_running_threads(sc);
    ssv6xxx_deinit_device(sc);
    printk("ieee80211_free_hw(): \n");
    ieee80211_free_hw(hw);
    pr_info("ssv6200: Driver unloaded\n");
    return 0;
}
EXPORT_SYMBOL(tu_ssv6xxx_dev_remove);
static const struct platform_device_id ssv6xxx_id_table[] = {
    {
        .name = SSV6200A,
        .driver_data = 0,
    },
    {
        .name = RSV6200A,
        .driver_data = 0,
    },
    #ifdef SSV_SUPPORT_SSV6006
    {
        .name = SSV6006A,
        .driver_data = 0,
    },
    {
        .name = SSV6006C,
        .driver_data = 0,
    },
    {
        .name = SSV6006D,
        .driver_data = 0,
    },
    #endif
    {},
};
MODULE_DEVICE_TABLE(platform, ssv6xxx_id_table);
static struct platform_driver ssv6xxx_driver =
{
    .probe = tu_ssv6xxx_dev_probe,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
    .remove = __devexit_p(tu_ssv6xxx_dev_remove),
#else
    .remove = tu_ssv6xxx_dev_remove,
#endif
    .id_table = ssv6xxx_id_table,
    .driver = {
        .name = SSV_DRVER_NAME,
        .owner = THIS_MODULE,
    }
};
static int device_match_by_alias(struct device *dev, void *data)
{
 struct device_driver *driver = dev->driver;
 struct _pattern {
  char driver_name[32];
  char device_name[32];
 } *pattern;
 pattern = (struct _pattern *)data;
 if (!strcmp(driver->name, pattern->driver_name) && !strcmp(dev_name(dev), pattern->device_name))
  return 1;
    if (!strcmp(driver->name, pattern->driver_name) && !strcmp("", pattern->device_name))
        return 1;
 else {
     printk("%s: driver[%s][%s], device[%s][%s]\n", __FUNCTION__, driver->name, pattern->driver_name,
      dev_name(dev), pattern->device_name);
  return 0;
    }
}
struct ssv_softc *ssv6xxx_driver_attach(char *driver_name)
{
 struct device *dev;
    struct ieee80211_hw *hw;
    struct ssv_softc *sc;
 struct _pattern {
  char driver_name[32];
  char device_name[32];
 } pattern;
    memset(&pattern, 0, sizeof(struct _pattern));
 sprintf(pattern.driver_name, "%s", driver_name);
 dev = driver_find_device(&ssv6xxx_driver.driver, NULL,
   (void *)&pattern, device_match_by_alias);
 if (!dev) {
  printk("Cannot find the driver[%s]\n", driver_name);
  return NULL;
 }
    hw = dev_get_drvdata(dev);
    sc = hw->priv;
    return sc;
}
void ssv6xxx_umac_hci_start(char *driver_name, char *device_name)
{
 struct device *dev;
    struct ieee80211_hw *hw;
    struct ssv_softc *sc;
 struct _pattern {
  char driver_name[32];
  char device_name[32];
 } pattern;
 sprintf(pattern.driver_name, "%s", driver_name);
 sprintf(pattern.device_name, "%s", device_name);
 dev = driver_find_device(&ssv6xxx_driver.driver, NULL,
   (void *)&pattern, device_match_by_alias);
 if (!dev) {
  printk("Cannot find the device[%s] driver[%s]\n", device_name, driver_name);
  return;
 }
 hw = dev_get_drvdata(dev);
 sc = hw->priv;
    HCI_STOP(sc->sh);
    HCI_START(sc->sh);
}
EXPORT_SYMBOL(ssv6xxx_umac_hci_start);
void ssv6xxx_umac_test(char *driver_name, char *device_name, struct sk_buff *skb, int size)
{
    struct device *dev;
    struct ieee80211_hw *hw;
    struct ssv_softc *sc;
    struct ssv_hw *sh;
    struct ssv6xxx_platform_data *priv;
    u32 ret;
        struct _pattern {
                char driver_name[32];
                char device_name[32];
        } pattern;
        sprintf(pattern.driver_name, "%s", driver_name);
        sprintf(pattern.device_name, "%s", device_name);
        dev = driver_find_device(&ssv6xxx_driver.driver, NULL,
                        (void *)&pattern, device_match_by_alias);
        if (!dev) {
                printk("Cannot find the device[%s] driver[%s]\n", device_name, driver_name);
                return;
        }
        hw = dev_get_drvdata(dev);
        sc = hw->priv;
        sh = sc->sh;
        priv = sh->priv;
        ret = priv->ops->load_fw(dev ,0x0 ,skb->data ,size);
}
EXPORT_SYMBOL(ssv6xxx_umac_test);
void ssv6xxx_umac_reg_read(char *driver_name, char *device_name, u32 addr, u32 *regval)
{
 int retval = 0;
 struct device *dev;
    struct ieee80211_hw *hw;
    struct ssv_softc *sc;
 struct _pattern {
  char driver_name[32];
  char device_name[32];
 } pattern;
 sprintf(pattern.driver_name, "%s", driver_name);
 sprintf(pattern.device_name, "%s", device_name);
 dev = driver_find_device(&ssv6xxx_driver.driver, NULL,
   (void *)&pattern, device_match_by_alias);
 if (!dev) {
  printk("Cannot find the device[%s] driver[%s]\n", device_name, driver_name);
  return;
 }
 hw = dev_get_drvdata(dev);
 sc = hw->priv;
    retval = SMAC_REG_READ(sc->sh, addr, regval);
 if (retval != 0)
  printk("Fail to read register");
}
EXPORT_SYMBOL(ssv6xxx_umac_reg_read);
void ssv6xxx_umac_reg_write(char *driver_name, char *device_name, u32 addr, u32 value)
{
 int retval = 0;
 struct device *dev;
    struct ieee80211_hw *hw;
    struct ssv_softc *sc;
 struct _pattern {
  char driver_name[32];
  char device_name[32];
 } pattern;
 sprintf(pattern.driver_name, "%s", driver_name);
 sprintf(pattern.device_name, "%s", device_name);
 dev = driver_find_device(&ssv6xxx_driver.driver, NULL,
   (void *)&pattern, device_match_by_alias);
 if (!dev) {
  printk("Cannot find the device[%s] driver[%s]\n", device_name, driver_name);
  return;
 }
 hw = dev_get_drvdata(dev);
 sc = hw->priv;
    retval = SMAC_REG_WRITE(sc->sh, addr, value);
 if (retval != 0)
  printk("Fail to read register");
}
EXPORT_SYMBOL(ssv6xxx_umac_reg_write);
void ssv6xxx_umac_tx_frame(char *driver_name, char *device_name, struct sk_buff *skb)
{
 struct device *dev;
    struct ieee80211_hw *hw;
    struct ssv_softc *sc;
 struct _pattern {
  char driver_name[32];
  char device_name[32];
 } pattern;
 sprintf(pattern.driver_name, "%s", driver_name);
 sprintf(pattern.device_name, "%s", device_name);
 dev = driver_find_device(&ssv6xxx_driver.driver, NULL,
   (void *)&pattern, device_match_by_alias);
 if (!dev) {
  printk("Cannot find the device[%s] driver[%s]\n", device_name, driver_name);
  return;
 }
 hw = dev_get_drvdata(dev);
 sc = hw->priv;
    HCI_SEND_CMD(sc->sh, skb);
}
EXPORT_SYMBOL(ssv6xxx_umac_tx_frame);
int ssv6xxx_umac_attach(char *driver_name, char *device_name, struct ssv6xxx_umac_ops *umac_ops)
{
 struct device *dev;
    struct ieee80211_hw *hw;
    struct ssv_softc *sc;
 struct _pattern {
  char driver_name[32];
  char device_name[32];
 } pattern;
 sprintf(pattern.driver_name, "%s", driver_name);
 sprintf(pattern.device_name, "%s", device_name);
 dev = driver_find_device(&ssv6xxx_driver.driver, NULL,
   (void *)&pattern, device_match_by_alias);
 if (!dev) {
  printk("Cannot find the device[%s] driver[%s]\n", device_name, driver_name);
  return -1;
 }
 hw = dev_get_drvdata(dev);
 sc = hw->priv;
 sc->umac = umac_ops;
 return 0;
}
EXPORT_SYMBOL(ssv6xxx_umac_attach);
int ssv6xxx_umac_deattach(char *driver_name, char *device_name)
{
 struct device *dev;
    struct ieee80211_hw *hw;
    struct ssv_softc *sc;
 struct _pattern {
  char driver_name[32];
  char device_name[32];
 } pattern;
 sprintf(pattern.driver_name, "%s", driver_name);
 sprintf(pattern.device_name, "%s", device_name);
 dev = driver_find_device(&ssv6xxx_driver.driver, NULL,
   (void *)&pattern, device_match_by_alias);
 if (!dev) {
  printk("Cannot find the device[%s] driver[%s]\n", device_name, driver_name);
  return -1;
 }
 hw = dev_get_drvdata(dev);
 sc = hw->priv;
 sc->umac = NULL;
 return 0;
}
EXPORT_SYMBOL(ssv6xxx_umac_deattach);
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
int tu_ssv6xxx_init(void)
#else
static int __init tu_ssv6xxx_init(void)
#endif
{
 int ret = 0;
#ifndef SSV_SUPPORT_HAL
    extern void *ssv_dbg_phy_table;
    extern u32 ssv_dbg_phy_len;
    extern void *ssv_dbg_rf_table;
    extern u32 ssv_dbg_rf_len;
    ssv_dbg_phy_table = (void *)ssv6200_phy_tbl;
    ssv_dbg_phy_len = sizeof(ssv6200_phy_tbl)/sizeof(ssv_cabrio_reg);
    ssv_dbg_rf_table = (void *)ssv6200_rf_tbl;
    ssv_dbg_rf_len = sizeof(ssv6200_rf_tbl)/sizeof(ssv_cabrio_reg);
#endif
    ret = ssv6xxx_rate_control_register();
    if (ret != 0) {
        printk("%s(): Failed to register rc algorithm.\n", __FUNCTION__);
  return ret;
    }
    return platform_driver_register(&ssv6xxx_driver);
}
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
void tu_ssv6xxx_exit(void)
#else
static void __exit tu_ssv6xxx_exit(void)
#endif
{
    ssv6xxx_rate_control_unregister();
    platform_driver_unregister(&ssv6xxx_driver);
}
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
EXPORT_SYMBOL(tu_ssv6xxx_init);
EXPORT_SYMBOL(tu_ssv6xxx_exit);
#else
module_init(tu_ssv6xxx_init);
module_exit(tu_ssv6xxx_exit);
#endif
