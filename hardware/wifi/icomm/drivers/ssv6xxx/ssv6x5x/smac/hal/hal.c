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

#ifdef ECLIPSE
#include <ssv_mod_conf.h>
#endif
#include <linux/version.h>
#include <linux/platform_device.h>
#ifdef SSV_SUPPORT_HAL
#include <linux/string.h>
#include <ssv6200.h>
#include <smac/dev.h>
#include <hal.h>
#include <smac/ssv_skb.h>
static int ssv6xxx_do_iq_cal(struct ssv_hw *sh, struct ssv6xxx_iqk_cfg *p_cfg)
{
 struct ssv_softc *sc = sh->sc;
 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "Do not need IQ CAL for this model!! \n");
 return 0;
}
static void ssv6xxx_dpd_enable(struct ssv_hw *sh, bool val)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "Not support DPD for this model!! \n");
}
static void tu_ssv6xxx_init_ch_cfg(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "Not support set channel dependant cfg for this model!! \n");
}
static void tu_ssv6xxx_init_iqk(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "Not support so save default iqk cfg for this model!! \n");
}
static void ssv6xxx_save_default_ipd_chcfg(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "Not support to change phy info according to ipd for this model!! \n");
}
static void ssv6xxx_chg_ipd_phyinfo(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "Not support to save default channel cfg for ipd for this model!! \n");
}
static void ssv6xxx_update_cfg_hw_patch(struct ssv_hw *sh,
    ssv_cabrio_reg *rf_table, ssv_cabrio_reg *phy_table)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_update_hw_config(struct ssv_hw *sh,
    ssv_cabrio_reg *rf_table, ssv_cabrio_reg *phy_table)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static int ssv6xxx_chg_pad_setting(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
    return 0;
}
static void ssv6xxx_cmd_cali(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_cmd_rc(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_cmd_efuse(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_set_sifs(struct ssv_hw *sh, int band)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_cmd_loopback(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_cmd_loopback_start(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_cmd_loopback_setup_env(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static int ssv6xxx_chk_lpbk_rx_rate_desc(struct ssv_hw *sh, struct sk_buff *skb)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
    return 0;
}
static void ssv6xxx_cmd_hwinfo(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static int ssv6xxx_get_sec_decode_err(struct sk_buff *skb, bool *mic_err, bool *decode_err)
{
    return 0;
}
static void ssv6xxx_cmd_cci(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_cmd_txgen(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_cmd_rf(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_cmd_hwq_limit(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_update_rf_pwr(struct ssv_softc *sc){
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void tu_ssv6xxx_init_gpio_cfg(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_flash_read_all_map(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_write_efuse(struct ssv_hw *sh, u8 *data, u8 data_length)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static int ssv6xxx_update_efuse_setting(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
    return 0;
}
static void ssv6xxx_update_product_hw_setting(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_do_temperature_compensation(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
void ssv6xxx_set_on3_enable(struct ssv_hw *sh, bool val)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_cmd_spectrum(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_wait_usb_rom_ready(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_detach_usb_hci(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_pll_chk(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     "%s is not supported for this model!!\n",__func__);
}
static void ssv6xxx_attach_common_hal (struct ssv_hal_ops *hal_ops)
{
    hal_ops->do_iq_cal = ssv6xxx_do_iq_cal;
    hal_ops->dpd_enable = ssv6xxx_dpd_enable;
    hal_ops->init_ch_cfg = tu_ssv6xxx_init_ch_cfg;
    hal_ops->init_iqk = tu_ssv6xxx_init_iqk;
    hal_ops->save_default_ipd_chcfg = ssv6xxx_save_default_ipd_chcfg;
    hal_ops->chg_ipd_phyinfo = ssv6xxx_chg_ipd_phyinfo;
    hal_ops->update_cfg_hw_patch = ssv6xxx_update_cfg_hw_patch;
    hal_ops->update_hw_config = ssv6xxx_update_hw_config;
    hal_ops->chg_pad_setting = ssv6xxx_chg_pad_setting;
    hal_ops->cmd_cali = ssv6xxx_cmd_cali;
    hal_ops->cmd_rc = ssv6xxx_cmd_rc;
    hal_ops->cmd_efuse = ssv6xxx_cmd_efuse;
    hal_ops->set_sifs = ssv6xxx_set_sifs;
 hal_ops->cmd_loopback = ssv6xxx_cmd_loopback;
 hal_ops->cmd_loopback_start = ssv6xxx_cmd_loopback_start;
 hal_ops->cmd_loopback_setup_env = ssv6xxx_cmd_loopback_setup_env;
    hal_ops->chk_lpbk_rx_rate_desc = ssv6xxx_chk_lpbk_rx_rate_desc;
 hal_ops->cmd_hwinfo = ssv6xxx_cmd_hwinfo;
 hal_ops->get_sec_decode_err = ssv6xxx_get_sec_decode_err;
 hal_ops->cmd_cci = ssv6xxx_cmd_cci;
 hal_ops->cmd_txgen = ssv6xxx_cmd_txgen;
 hal_ops->cmd_rf = ssv6xxx_cmd_rf;
 hal_ops->cmd_hwq_limit = ssv6xxx_cmd_hwq_limit;
 hal_ops->update_rf_pwr = ssv6xxx_update_rf_pwr;
    hal_ops->init_gpio_cfg = tu_ssv6xxx_init_gpio_cfg;
    hal_ops->flash_read_all_map = ssv6xxx_flash_read_all_map;
    hal_ops->write_efuse = ssv6xxx_write_efuse;
    hal_ops->update_efuse_setting = ssv6xxx_update_efuse_setting;
 hal_ops->do_temperature_compensation = ssv6xxx_do_temperature_compensation;
    hal_ops->update_product_hw_setting = ssv6xxx_update_product_hw_setting;
    hal_ops->set_on3_enable = ssv6xxx_set_on3_enable;
    hal_ops->cmd_spectrum = ssv6xxx_cmd_spectrum;
    hal_ops->wait_usb_rom_ready = ssv6xxx_wait_usb_rom_ready;
    hal_ops->detach_usb_hci = ssv6xxx_detach_usb_hci;
    hal_ops->pll_chk = ssv6xxx_pll_chk;
}
int tu_ssv6xxx_init_hal(struct ssv_softc *sc)
{
    struct ssv_hw *sh;
    int ret = 0;
    struct ssv_hal_ops *hal_ops = NULL;
    extern void ssv_attach_ssv6051(struct ssv_softc *sc, struct ssv_hal_ops *hal_ops);
    extern void ssv_attach_ssv6006(struct ssv_softc *sc, struct ssv_hal_ops *hal_ops);
    bool chip_supportted = false;
	struct ssv6xxx_platform_data *priv = sc->dev->platform_data;
 hal_ops = kzalloc(sizeof(struct ssv_hal_ops), GFP_KERNEL);
 if (hal_ops == NULL) {
  printk("%s(): Fail to alloc hal_ops\n", __FUNCTION__);
  return -ENOMEM;
 }
    ssv6xxx_attach_common_hal(hal_ops);
#ifdef SSV_SUPPORT_SSV6051
    if ( strstr(priv->chip_id, SSV6051_CHIP)
        || strstr(priv->chip_id, SSV6051_CHIP_ECO3)) {
        printk(KERN_INFO"Attach SSV6051 family HAL function \n");
        ssv_attach_ssv6051(sc, hal_ops);
        chip_supportted = true;
    }
#endif
#ifdef SSV_SUPPORT_SSV6006
    if ( strstr(priv->chip_id, SSV6006)
          || strstr(priv->chip_id, SSV6006C)
          || strstr(priv->chip_id, SSV6006D)) {
        printk(KERN_INFO"Attach SSV6006 family HAL function  \n");
        ssv_attach_ssv6006(sc, hal_ops);
        chip_supportted = true;
    }
#endif
    if (!chip_supportted) {
        printk(KERN_ERR "Chip \"%s\" is not supported by this driver\n", priv->chip_id);
        ret = -EINVAL;
        goto out;
    }
    sh = hal_ops->alloc_hw();
    if (sh == NULL) {
        ret = -ENOMEM;
        goto out;
    }
    memcpy(&sh->hal_ops, hal_ops, sizeof(struct ssv_hal_ops));
    sc->sh = sh;
    sh->sc = sc;
    INIT_LIST_HEAD(&sh->hw_cfg);
    mutex_init(&sh->hw_cfg_mutex);
    sh->priv = sc->dev->platform_data;
    sh->hci.dev = sc->dev;
    sh->hci.if_ops = sh->priv->ops;
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
out:
 kfree(hal_ops);
    return ret;
}
#endif
