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

#ifndef _HAL_H_
#define _HAL_H_ 
#ifdef SSV_SUPPORT_HAL
extern u32 phy_info_tbl[];
extern size_t phy_info_tbl_size;
#include <smac/ssv_reg_acc.h>
enum ssv6xxx_beacon_type{
 SSV6xxx_BEACON_0,
 SSV6xxx_BEACON_1,
};
#define EFUSE_HWSET_MAX_SIZE (256-32)
#define EFUSE_MAX_SECTION_MAP (EFUSE_HWSET_MAX_SIZE>>5)
#define BEACON_WAITING_ENABLED 1<<0
#define BEACON_ENABLED 1<<1
#define MAX_FAIL_COUNT 100
#define MAX_RETRY_COUNT 20
#define HW_ID_OFFSET 7
#define ADDRESS_OFFSET 16
int tu_ssv6xxx_init_hal(struct ssv_softc *sc);
int ssvxxx_get_sta_assco_cnt(struct ssv_softc *sc);
#define HAL_ADJ_CONFIG(_sh) _sh->hal_ops.adj_config(_sh)
#define HAL_NEED_SW_CIPHER(_sh) _sh->hal_ops.need_sw_cipher(_sh)
#define HAL_INIT_MAC(_sh) _sh->hal_ops.init_mac(_sh)
#define HAL_RESET_SYSPLF(_sh) _sh->hal_ops.reset_sysplf(_sh)
#define HAL_INI_HW_SEC_PHY_TABLE(_sc) _sc->sh->hal_ops.init_hw_sec_phy_table(_sc)
#define HAL_INIT_IQK(_sh) _sh->hal_ops.init_iqk(_sh)
#define HAL_WRITE_MAC_INI(_sh) _sh->hal_ops.write_mac_ini(_sh)
#define HAL_USE_HW_ENCRYPT(_cipher,_sc,_sta_priv,_vif_priv) \
                _sc->sh->hal_ops.use_hw_encrypt(_cipher, _sc, _sta_priv, _vif_priv)
#define HAL_SET_RX_FLOW(_sh,_type,_rxflow) \
                _sh->hal_ops.set_rx_flow(_sh, _type, _rxflow)
#define HAL_SET_RX_CTRL_FLOW(_sh) _sh->hal_ops.set_rx_ctrl_flow(_sh)
#define HAL_SET_MACADDR(_sh,_vif_idx) _sh->hal_ops.set_macaddr(_sh, _vif_idx)
#define HAL_SET_BSSID(_sh,_bssid,_vif_idx) \
                _sh->hal_ops.set_bssid(_sh, _bssid, _vif_idx)
#define HAL_GET_IC_TIME_TAG(_sh) _sh->hal_ops.get_ic_time_tag(_sh)
#define HAL_GET_CHIP_ID(_sh) _sh->hal_ops.get_chip_id(_sh)
#define HAL_IF_CHK_MAC2(_sh) _sh->hal_ops.if_chk_mac2(_sh)
#define HAL_SAVE_HW_STATUS(_sc) _sc->sh->hal_ops.save_hw_status( _sc)
#define HAL_PLL_CHK(_sh) _sh->hal_ops.pll_chk(_sh)
#define HAL_GET_WSID(_sc,_vif,_sta) _sc->sh->hal_ops.get_wsid( _sc, _vif, _sta)
#define HAL_SET_HW_WSID(_sc,_vif,_sta,_wsid) \
                _sc->sh->hal_ops.set_hw_wsid( _sc, _vif, _sta, _wsid)
#define HAL_DEL_HW_WSID(_sc,_hw_wsid) _sc->sh->hal_ops.del_hw_wsid( _sc, _hw_wsid)
#define HAL_ADD_FW_WSID(_sc,_vif_priv,_sta,_sta_info) \
                _sc->sh->hal_ops.add_fw_wsid( _sc, _vif_priv, _sta, _sta_info)
#define HAL_DEL_FW_WSID(_sc,_sta,_sta_info) \
                _sc->sh->hal_ops.del_fw_wsid( _sc, _sta, _sta_info)
#define HAL_SET_FW_HWWSID_SEC_TYPE(_sc,_sta,_sta_info,_vif_priv) \
                _sc->sh->hal_ops.set_fw_hwwsid_sec_type( _sc, _sta, _sta_info, _vif_priv)
#define HAL_ENABLE_FW_WSID(_sc,_sta,_sta_info,_key_type) \
                _sc->sh->hal_ops.enable_fw_wsid( _sc, _sta, _sta_info, _key_type)
#define HAL_DISABLE_FW_WSID(_sc,_key_idx,_sta_priv,_vif_priv) \
                _sc->sh->hal_ops.disable_fw_wsid( _sc, _key_idx, _sta_priv, _vif_priv)
#define HAL_WEP_USE_HW_CIPHER(_sc,_vif_priv) \
                 _sc->sh->hal_ops.wep_use_hw_cipher( _sc, _vif_priv)
#define HAL_PAIRWISE_WPA_USE_HW_CIPHER(_sc,_vif_priv,_cipher,_sta_priv) \
                _sc->sh->hal_ops.pairwise_wpa_use_hw_cipher( _sc, _vif_priv, _cipher, _sta_priv)
#define HAL_GROUP_WPA_USE_HW_CIPHER(_sc,_vif_priv,_cipher) \
                _sc->sh->hal_ops.group_wpa_use_hw_cipher( _sc, _vif_priv, _cipher)
#define HAL_SET_AES_TKIP_HW_CRYPTO_GROUP_KEY(_sc,_vif_info,_sta_info,_param) \
                _sc->sh->hal_ops.set_aes_tkip_hw_crypto_group_key(_sc, _vif_info, _sta_info, _param)
#define HAL_WRITE_PAIRWISE_KEYIDX_TO_HW(_sh,_key_idx,_wsid) \
                _sh->hal_ops.write_pairwise_keyidx_to_hw(_sh, _key_idx, _wsid)
#define HAL_WRITE_GROUP_KEYIDX_TO_HW(_sh,_vif_priv,_key_idx) \
                _sh->hal_ops.write_group_keyidx_to_hw(_sh, _vif_priv, _key_idx)
#define HAL_WRITE_PAIRWISE_KEY_TO_HW(_sc,_key_idx,_alg,_key,_key_len,_keyconf,_vif_priv,_sta_priv) \
                _sc->sh->hal_ops.write_pairwise_key_to_hw(_sc, _key_idx, _alg, _key, _key_len, _keyconf, _vif_priv, _sta_priv)
#define HAL_WRITE_GROUP_KEY_TO_HW(_sc,_key_idx,_alg,_key,_key_len,_keyconf,_vif_priv,_sta_priv) \
                _sc->sh->hal_ops.write_group_key_to_hw(_sc, _key_idx, _alg, _key, _key_len, _keyconf, _vif_priv, _sta_priv)
#define HAL_WRITE_KEY_TO_HW(_sc,_vif_priv,_sram_ptr,_wsid,_key_idx,_key_type) \
                _sc->sh->hal_ops.write_key_to_hw(_sc, _vif_priv, _sram_ptr, _wsid, _key_idx, _key_type)
#define HAL_SET_GROUP_CIPHER_TYPE(_sh,_vif_priv,_cipher) \
                _sh->hal_ops.set_group_cipher_type( _sh, _vif_priv, _cipher)
#define HAL_SET_PAIRWISE_CIPHER_TYPE(_sh,_cipher,_wsid) \
                _sh->hal_ops.set_pairwise_cipher_type( _sh, _cipher, _wsid)
#define HAL_CHK_IF_SUPPORT_HW_BSSID(_sc,_vif_idx) \
                _sc->sh->hal_ops.chk_if_support_hw_bssid( _sc, _vif_idx)
#define HAL_CHK_DUAL_VIF_CHG_RX_FLOW(_sc,_vif_priv) \
                _sc->sh->hal_ops.chk_dual_vif_chg_rx_flow( _sc, _vif_priv)
#define HAL_RESTORE_RX_FLOW(_sc,_vif_priv,_sta) \
                _sc->sh->hal_ops.restore_rx_flow( _sc, _vif_priv, _sta)
#define HAL_HW_CRYPTO_KEY_WRITE_WEP(_sc,_keyconf,_alg,_vif_info) \
                _sc->sh->hal_ops.hw_crypto_key_write_wep( _sc, _keyconf, _alg, _vif_info)
#define HAL_SET_WEP_HW_CRYPTO_KEY(_sc,_sta_info,_vif_priv) \
                _sc->sh->hal_ops.set_wep_hw_crypto_key( _sc, _sta_info, _vif_priv)
#define HAL_STORE_WEP_KEY(_sc,_vif_priv,_sta_priv,_cipher,_key) \
                _sc->sh->hal_ops.store_wep_key( _sc, _vif_priv, _sta_priv, _cipher, _key)
#define HAL_PUT_MIC_SPACE_FOR_HW_CCMP_ENCRYPT(_sc,_skb) \
                _sc->sh->hal_ops.put_mic_space_for_hw_ccmp_encrypt( _sc, _skb)
#ifdef CONFIG_PM
#define HAL_SAVE_CLEAR_TRAP_REASON(_sc) \
                _sc->sh->hal_ops.save_clear_trap_reason(_sc)
#define HAL_RESTORE_TRAP_REASON(_sc) \
                _sc->sh->hal_ops.restore_trap_reason(_sc)
#define HAL_PMU_AWAKE(_sc) \
                _sc->sh->hal_ops.pmu_awake(_sc)
#endif
#define HAL_SET_REPLAY_IGNORE(_sh,_ignore) \
                _sh->hal_ops.set_replay_ignore(_sh, _ignore)
#define HAL_UPDATE_DECISION_TABLE_6(_sh,_val) \
                _sh->hal_ops.update_decision_table_6(_sh, _val)
#define HAL_UPDATE_DECISION_TABLE(_sc) _sc->sh->hal_ops.update_decision_table(_sc)
#define HAL_GET_FW_VERSION(_sh,_regval) \
                _sh->hal_ops.get_fw_version(_sh, _regval)
#define HAL_SET_MRX_MODE(_sh,_regval) _sh->hal_ops.set_mrx_mode(_sh, _regval)
#define HAL_GET_MRX_MODE(_sh,_regval) _sh->hal_ops.get_mrx_mode(_sh, _regval)
#define HAL_SET_OP_MODE(_sh,_op_mode,_vif_idx) _sh->hal_ops.set_op_mode(_sh, _op_mode, _vif_idx)
#define HAL_HALT_MNGQ_UNTIL_DTIM(_sh,_val) \
                _sh->hal_ops.set_halt_mngq_util_dtim(_sh,_val)
#define HAL_SET_DUR_BURST_SIFS_G(_sh,_val) \
                _sh->hal_ops.set_dur_burst_sifs_g(_sh, _val)
#define HAL_SET_DUR_SLOT(_sh,_val) _sh->hal_ops.set_dur_slot(_sh, _val)
#define HAL_SET_SIFS(_sh,_band) _sh->hal_ops.set_sifs(_sh, _band)
#define HAL_SET_QOS_ENABLE(_sh,_val) _sh->hal_ops.set_qos_enable(_sh, _val)
#define HAL_SET_WMM_PARAM(_sc,_params,_queue) \
                _sc->sh->hal_ops.set_wmm_param(_sc, _params, _queue)
#define HAL_UPDATE_PAGE_ID(_sh) _sh->hal_ops.update_page_id(_sh)
#define HAL_INIT_TX_CFG(_sh) _sh->hal_ops.init_tx_cfg(_sh)
#define HAL_INIT_RX_CFG(_sh) _sh->hal_ops.init_rx_cfg(_sh)
#define HAL_ALLOC_PBUF(_sc,_size,_type) \
                _sc->sh->hal_ops.alloc_pbuf(_sc, _size, _type)
#define HAL_FREE_PBUF(_sc,_addr) _sc->sh->hal_ops.free_pbuf(_sc, _addr)
#define HAL_AMPDU_AUTO_CRC_EN(_sh) _sh->hal_ops.ampdu_auto_crc_en(_sh)
#define HAL_SET_RX_BA(_sh,_on,_ta,_tid,_ssn,_buf_size) \
                _sh->hal_ops.set_rx_ba(_sh, _on, _ta, _tid, _ssn, _buf_size)
#define HAL_READ_EFUSE(_sh,_pbuf) _sh->hal_ops.read_efuse(_sh, _pbuf)
#define HAL_WRITE_EFUSE(_sh,_data,_len) _sh->hal_ops.write_efuse(_sh, _data, _len)
#define HAL_BEACON_GET_VALID_CFG(_sh) _sh->hal_ops.beacon_get_valid_cfg(_sh)
#define HAL_SET_BEACON_REG_LOCK(_sh,_val) \
                                       _sh->hal_ops.set_beacon_reg_lock(_sh, _val)
#define HAL_SET_BCN_ID_DTIM(_sc,_bcntype,_dtim) \
                _sc->sh->hal_ops.set_beacon_id_dtim(_sc, _bcntype, _dtim)
#define HAL_FILL_BCN(_sc,_regaddr,_skb) \
                _sc->sh->hal_ops.fill_beacon(_sc, _regaddr, _skb)
#define HAL_BEACON_ENABLE(_sc,_val) _sc->sh->hal_ops.beacon_enable(_sc, _val)
#define HAL_SET_BCN_IFNO(_sh,_interval,_cnt) \
                _sh->hal_ops.set_beacon_info(_sh, _interval, _cnt)
#define HAL_GET_BCN_ONGOING(_sh) _sh->hal_ops.get_bcn_ongoing(_sh)
#define HAL_BEACON_LOSS_ENABLE(_sh) _sh->hal_ops.beacon_loss_enable(_sh)
#define HAL_BEACON_LOSS_DISABLE(_sh) _sh->hal_ops.beacon_loss_disable(_sh)
#define HAL_BEACON_LOSS_CONFIG(_sh,_beacon_int,_bssid) \
    _sh->hal_ops.beacon_loss_config(_sh, _beacon_int, _bssid)
#define HAL_UPDATE_TXQ_MASK(_sh,_txq_mask) \
                _sh->hal_ops.update_txq_mask(_sh, _txq_mask)
#define HAL_READRG_HCI_INQ_INFO(_sh,_hci_used_id,_tx_use_page) \
                _sh->hal_ops.readrg_hci_inq_info(_sh, _hci_used_id, _tx_use_page)
#define HAL_READRG_TXQ_INFO(_sh,_txq_info,_hci_used_id) \
                _sh->hal_ops.readrg_txq_info(_sh, _txq_info, _hci_used_id)
#define HAL_READRG_TXQ_INFO2(_sh,_txq_info2,_hci_used_id) \
                _sh->hal_ops.readrg_txq_info2(_sh, _txq_info2, _hci_used_id)
#define HAL_DUMP_WSID(_sh) \
                _sh->hal_ops.dump_wsid(_sh)
#define HAL_DUMP_DECISION(_sh) \
                _sh->hal_ops.dump_decision(_sh)
#define HAL_DUMP_PHY_REG(_sh) \
                _sh->hal_ops.dump_phy_reg(_sh)
#define HAL_DUMP_RF_REG(_sh) \
                _sh->hal_ops.dump_rf_reg(_sh)
#define HAL_GET_FFOUT_CNT(_sh,_value,_tag) \
    _sh->hal_ops.get_ffout_cnt(_value, _tag)
#define HAL_GET_IN_FFCNT(_sh,_value,_tag) \
    _sh->hal_ops.get_in_ffcnt(_value, _tag)
#define HAL_READ_FFOUT_CNT(_sh,_value,_value1,_value2) \
                _sh->hal_ops.read_ffout_cnt(_sh, _value, _value1, _value2)
#define HAL_READ_IN_FFCNT(_sh,_value,_value1) \
                _sh->hal_ops.read_in_ffcnt(_sh, _value, _value1)
#define HAL_READ_ID_LEN_THRESHOLD(_sh,_tx_len,_rx_len) \
                _sh->hal_ops.read_id_len_threshold(_sh, _tx_len, _rx_len)
#define HAL_READ_TAG_STATUS(_sh,_ava_status) \
                _sh->hal_ops.read_tag_status(_sh, _ava_status)
#define HAL_CMD_MIB(_sc,_argc,_argv) _sc->sh->hal_ops.cmd_mib(_sc, _argc, _argv)
#define HAL_CMD_POWER_SAVING(_sc,_argc,_argv) _sc->sh->hal_ops.cmd_power_saving(_sc, _argc, _argv)
#define HAL_SUPPORT_IQK_CMD(_sh) _sh->hal_ops.support_iqk_cmd(_sh)
#define HAL_GET_RD_ID_ADR(_sh,_id_adr) \
                _sh->hal_ops.get_rd_id_adr(_id_adr)
#define HAL_CMD_CALI(_sh,_argc,_argv) \
                _sh->hal_ops.cmd_cali(_sh, _argc, _argv)
#define HAL_BURST_READ_REG(_sh,_reg,_val,_num) \
                _sh->hal_ops.burst_read_reg(_sh, _reg, _val, _num)
#define HAL_BURST_WRITE_REG(_sh,_reg,_val,_num) \
                _sh->hal_ops.burst_write_reg(_sh, _reg, _val, _num)
#define HAL_AUTO_GEN_NULLPKT(_sh,_hwq) \
    _sh->hal_ops.auto_gen_nullpkt(_sh, _hwq)
#define HAL_RC_ALGORITHM(_sc) _sc->sh->hal_ops.rc_algorithm(_sc)
#define HAL_SET_80211HW_RATE_CONFIG(_sc) \
     _sc->sh->hal_ops.set_80211_hw_rate_config(_sc)
#define HAL_RC_LEGACY_BITRATE_TO_RATE_DESC(_sc,_bitrate,_drate) \
     _sc->sh->hal_ops.rc_legacy_bitrate_to_rate_desc(_bitrate, _drate)
#define HAL_RC_RX_DATA_HANDLER(_sc,_skb,rate_idx) \
     _sc->sh->hal_ops.rc_rx_data_handler(_sc, _skb, rate_idx)
#define HAL_RATE_REPORT_HANDLER(_sc,_skb,_no_ba_result) \
     _sc->sh->hal_ops.rate_report_handler(_sc, _skb, _no_ba_result)
#define HAL_RC_PROCESS_RATE_REPORT(_sc,_skb) \
     _sc->sh->hal_ops.rc_process_rate_report(_sc, _skb)
#define HAL_RC_UPDATE_BASIC_RATE(_sc,_basic_rates) \
     _sc->sh->hal_ops.rc_update_basic_rate(_sc, _basic_rates)
#define HAL_HT_RATE_UPDATE(_sc,_skb,_rates) \
     _sc->sh->hal_ops.rc_ht_update_rate(_skb, _sc, _rates)
#define HAL_RC_HT_STA_CURRENT_RATE_IS_CCK(_sc,_sta) \
     _sc->sh->hal_ops.rc_ht_sta_current_rate_is_cck(_sta)
#define HAL_LOAD_FW_ENABLE_MCU(_sh) _sh->hal_ops.load_fw_enable_mcu(_sh)
#define HAL_LOAD_FW_DISABLE_MCU(_sh) _sh->hal_ops.load_fw_disable_mcu(_sh)
#define HAL_LOAD_FW_SET_STATUS(_sh,_status) \
           _sh->hal_ops.load_fw_set_status(_sh, _status)
#define HAL_LOAD_FW_GET_STATUS(_sh,_status) \
           _sh->hal_ops.load_fw_get_status(_sh, _status)
#define HAL_LOAD_FW_PRE_CONFIG_DEVICE(_sh) \
           _sh->hal_ops.load_fw_pre_config_device(_sh)
#define HAL_LOAD_FW_POST_CONFIG_DEVICE(_sh) \
           _sh->hal_ops.load_fw_post_config_device(_sh)
#define HAL_RESET_CPU(_sh) _sh->hal_ops.reset_cpu(_sh)
#define HAL_SET_SRAM_MODE(_sh,_mode) _sh->hal_ops.set_sram_mode(_sh, _mode)
#define HAL_ENABLE_USB_ACC(_sc,_epnum) _sc->sh->hal_ops.enable_usb_acc(_sc, _epnum)
#define HAL_DISABLE_USB_ACC(_sc,_epnum) _sc->sh->hal_ops.disable_usb_acc(_sc, _epnum)
#define HAL_SET_USB_LPM(_sc,_enable) _sc->sh->hal_ops.set_usb_lpm(_sc, _enable)
#define HAL_JUMP_TO_ROM(_sc) _sc->sh->hal_ops.jump_to_rom(_sc)
#define HAL_GET_FW_NAME(_sh,_name) _sh->hal_ops.get_fw_name(_name)
#define HAL_SEND_TX_POLL_CMD(_sh,_type) _sh->hal_ops.send_tx_poll_cmd(_sh, _type)
#define HAL_FLASH_READ_ALL_MAP(_sh) _sh->hal_ops.flash_read_all_map(_sh)
#define HAL_WAIT_USB_ROM_READY(_sh) _sh->hal_ops.wait_usb_rom_ready(_sh)
#define HAL_DETACH_USB_HCI(_sh) _sh->hal_ops.detach_usb_hci(_sh)
#define HAL_ADD_TXINFO(_sc,_skb) _sc->sh->hal_ops.add_txinfo( _sc, _skb)
#define HAL_UPDATE_TXINFO(_sc,_skb) _sc->sh->hal_ops.update_txinfo( _sc, _skb)
#define HAL_UPDATE_AMPDU_TXINFO(_sc,_skb) \
                                       _sc->sh->hal_ops.update_ampdu_txinfo( _sc, _skb)
#define HAL_ADD_AMPDU_TXINFO(_sc,_skb) \
                                       _sc->sh->hal_ops.add_ampdu_txinfo( _sc, _skb)
#define HAL_UPDATE_NULL_FUNC_TXINFO(_sc,_sta,_skb) \
                                       _sc->sh->hal_ops.update_null_func_txinfo( _sc, _sta, _skb)
#define HAL_GET_TX_DESC_SIZE(_sh) _sh->hal_ops.get_tx_desc_size(_sh)
#define HAL_GET_TX_DESC_CTYPE(_sh,_skb) \
                                       _sh->hal_ops.get_tx_desc_ctype(_skb)
#define HAL_GET_TX_DESC_REASON(_sh,_skb) \
                                       _sh->hal_ops.get_tx_desc_reason(_skb)
#define HAL_GET_TX_DESC_TXQ_IDX(_sh,_skb) \
                                       _sh->hal_ops.get_tx_desc_txq_idx(_skb)
#define HAL_TX_RATE_UPDATE(_sc,_skb) _sc->sh->hal_ops.tx_rate_update( _sc, _skb)
#define HAL_TXTPUT_SET_DESC(_sh,_skb) _sh->hal_ops.txtput_set_desc( _sh, _skb)
#define HAL_FILL_BEACON_TX_DESC(_sc,_skb) \
                _sc->sh->hal_ops.fill_beacon_tx_desc(_sc, _skb)
#define HAL_FILL_LPBK_TX_DESC(_sc,_skb,_sec,_rate) \
                _sc->sh->hal_ops.fill_lpbk_tx_desc(_skb, _sec, _rate)
#define HAL_CHK_LPBK_RX_RATE_DESC(_sh,_skb) \
                                       _sh->hal_ops.chk_lpbk_rx_rate_desc(_sh, _skb)
#define HAL_GET_SEC_DECODE_ERR(_sh,_skb,_mic_err,_decode_err) \
                                       _sh->hal_ops.get_sec_decode_err(_skb, _mic_err, _decode_err)
#define HAL_GET_RX_DESC_SIZE(_sh) _sh->hal_ops.get_rx_desc_size(_sh)
#define HAL_GET_RX_DESC_LENGTH(_sh) _sh->hal_ops.get_rx_desc_length(_sh)
#define HAL_GET_RX_DESC_WSID(_sh,_skb) \
                                       _sh->hal_ops.get_rx_desc_wsid( _skb)
#define HAL_GET_RX_DESC_RATE_IDX(_sh,_skb) \
                                       _sh->hal_ops.get_rx_desc_rate_idx( _skb)
#define HAL_GET_RX_DESC_MNG_USED(_sh,_skb) \
                                       _sh->hal_ops.get_rx_desc_mng_used( _skb)
#define HAL_IS_RX_AGGR(_sh,_skb) _sh->hal_ops.is_rx_aggr( _skb)
#define HAL_GET_RX_DESC_CTYPE(_sh,_skb) \
                                       _sh->hal_ops.get_rx_desc_ctype( _skb)
#define HAL_GET_RX_DESC_HDR_OFFSET(_sh,_skb) \
                                       _sh->hal_ops.get_rx_desc_hdr_offset( _skb)
#define HAL_GET_RX_DESC_INFO(_sh,_skb,_packet_len,_c_type,_tx_pkt_run_no) \
                _sh->hal_ops.get_rx_desc_info( _skb, _packet_len, _c_type, _tx_pkt_run_no)
#define HAL_NULLFUN_FRAME_FILTER(_sh,_skb) \
                _sh->hal_ops.nullfun_frame_filter(_skb)
#define HAL_SET_PHY_MODE(_sh,_val) _sh->hal_ops.set_phy_mode(_sh, _val)
#define HAL_PHY_ENABLE(_sh,_val) _sh->hal_ops.phy_enable(_sh, _val)
#define HAL_EDCA_ENABLE(_sh,_val) _sh->hal_ops.edca_enable(_sh, _val)
#define HAL_EDCA_STAT(_sh) _sh->hal_ops.edca_stat(_sh)
#define HAL_RESET_MIB_PHY(_sh) _sh->hal_ops.reset_mib_phy(_sh)
#define HAL_DUMP_MIB_RX_PHY(_sh) \
                _sh->hal_ops.dump_mib_rx_phy(_sh)
#define HAL_RC_MAC80211_RATE_IDX(_sc,_rate_idx,_rxs) \
                _sc->sh->hal_ops.rc_mac80211_rate_idx(_sc, _rate_idx, _rxs)
#define HAL_UPDATE_RXSTATUS(_sc,_skb,_rxs) \
                _sc->sh->hal_ops.update_rxstatus(_sc, _skb, _rxs)
#ifdef CONFIG_SSV_CCI_IMPROVEMENT
#define HAL_UPDATE_SCAN_CCI_SETTING(_sc) _sc->sh->hal_ops.update_scan_cci_setting(_sc)
#define HAL_RECOVERY_SCAN_CCI_SETTING(_sc) _sc->sh->hal_ops.recover_scan_cci_setting(_sc)
#endif
#define HAL_CMD_RC(_sh,_argc,_argv) _sh->hal_ops.cmd_rc(_sh, _argc, _argv)
#define HAL_CMD_EFUSE(_sh,_argc,_argv) _sh->hal_ops.cmd_efuse(_sh, _argc, _argv)
#define HAL_CMD_SPECTRUM(_sh) _sh->hal_ops.cmd_spectrum(_sh)
#define HAL_CMD_LOOPBACK(_sh,_argc,_argv) _sh->hal_ops.cmd_loopback(_sh, _argc, _argv)
#define HAL_CMD_LOOPBACK_START(_sh) _sh->hal_ops.cmd_loopback_start(_sh)
#define HAL_CMD_LOOPBACK_SETUP_ENV(_sh) _sh->hal_ops.cmd_loopback_setup_env(_sh)
#define HAL_CMD_HWINFO(_sh,_argc,_argv) _sh->hal_ops.cmd_hwinfo(_sh, _argc, _argv)
#define HAL_CMD_CCI(_sh,_argc,_argv) _sh->hal_ops.cmd_cci(_sh, _argc, _argv)
#define HAL_CMD_TXGEN(_sh) _sh->hal_ops.cmd_txgen(_sh)
#define HAL_CMD_RF(_sh,_argc,_argv) _sh->hal_ops.cmd_rf(_sh, _argc, _argv)
#define HAL_UPDATE_RF_PWR(_sc) _sc->sh->hal_ops.update_rf_pwr(_sc)
#define HAL_CMD_HWQ_LIMIT(_sh,_argc,_argv) _sh->hal_ops.cmd_hwq_limit(_sh, _argc, _argv)
#define HAL_AMPDU_RX_START(_sc,_hw,_vif,_sta,_tid,_ssn,_buf_size) \
                _sc->sh->hal_ops.ampdu_rx_start(_hw, _vif, _sta, _tid, _ssn, _buf_size)
#define HAL_AMPDU_BA_HANDLER(_sc,_hw,_skb,_tx_pkt_run_no) \
                _sc->sh->hal_ops.ampdu_ba_handler( _hw, _skb, _tx_pkt_run_no)
#define HAL_IS_LEGACY_RATE(_sc,_skb) _sc->sh->hal_ops.is_legacy_rate(_sc, _skb)
#define HAL_AMPDU_MAX_TRANSMIT_LENGTH(_sc,_skb,_rate_idx) \
    _sc->sh->hal_ops.ampdu_max_transmit_length(_sc, _skb, _rate_idx)
#define HAL_LOAD_PHY_TABLE(_sh,_tbl) _sh->hal_ops.load_phy_table(_tbl)
#define HAL_GET_PHY_TABLE_SIZE(_sh) _sh->hal_ops.get_phy_table_size(_sh)
#define HAL_LOAD_RF_TABLE(_sh,_tbl) _sh->hal_ops.load_rf_table(_tbl)
#define HAL_GET_RF_TABLE_SIZE(_sh) _sh->hal_ops.get_rf_table_size(_sh)
#define HAL_INIT_PLL(_sh) _sh->hal_ops.init_pll(_sh)
#define HAL_UPDATE_CFG_HW_PATCH(_sh,_rftbl,_phytbl) \
                                       _sh->hal_ops.update_cfg_hw_patch(_sh, _rftbl, _phytbl)
#define HAL_UPDATE_HW_CONFIG(_sh,_rftbl,_phytbl) \
                                       _sh->hal_ops.update_hw_config(_sh, _rftbl, _phytbl)
#define HAL_CHG_PAD_SETTING(_sh) _sh->hal_ops.chg_pad_setting(_sh)
#define HAL_CHG_CLK_SRC(_sh) _sh->hal_ops.chg_clk_src(_sh)
#define HAL_UPDATE_EFUSE_SETTING(_sh) _sh->hal_ops.update_efuse_setting(_sh)
#define HAL_DO_TEMPERATURE_COMPENSATION(_sh) \
                                       _sh->hal_ops.do_temperature_compensation(_sh)
#define HAL_UPDATE_PRODUCT_HW_SETTING(_sh) _sh->hal_ops.update_product_hw_setting(_sh)
#define HAL_SET_CHANNEL(_sc,_ch,_type) _sc->sh->hal_ops.set_channel(_sc, _ch, _type)
#define HAL_DO_IQ_CAL(_sh,_pcfg) _sh->hal_ops.do_iq_cal(_sh,_pcfg)
#define HAL_SET_PLL_PHY_RF(_sh,_rf_tbl,_phy_tbl) \
                                       _sh->hal_ops.set_pll_phy_rf(_sh, _rf_tbl, _phy_tbl)
#define HAL_DPD_ENABLE(_sh,_val) _sh->hal_ops.dpd_enable(_sh, _val)
#define HAL_INIT_CH_CFG(_sh) _sh->hal_ops.init_ch_cfg(_sh)
#define HAL_SAVE_DEFAULT_IPD_CHCFG(_sh) \
                                       _sh->hal_ops.save_default_ipd_chcfg(_sh)
#define HAL_CHG_IPD_PHYINFO(_sh) _sh->hal_ops.chg_ipd_phyinfo(_sh)
#define HAL_SET_RF_ENABLE(_sh,_val) _sh->hal_ops.set_rf_enable(_sh, _val)
#define HAL_SET_ON3_ENABLE(_sh,_val) _sh->hal_ops.set_on3_enable(_sh, _val)
#define HAL_INIT_GPIO_CFG(_sh) _sh->hal_ops.init_gpio_cfg(_sh)
#endif
#endif
