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

#ifndef _SSV_HCI_H_
#define _SSV_HCI_H_ 
#define SSV_SC(_ctrl_hci) (_ctrl_hci->shi->sc)
#define TX_PAGE_NOT_LIMITED 255
#define SSV_HW_TXQ_NUM 5
#define SSV_HW_TXQ_MAX_SIZE 64
#define SSV_HW_TXQ_RESUME_THRES ((SSV_HW_TXQ_MAX_SIZE >> 2) *3)
#define SSV6XXX_ID_TX_THRESHOLD(_hctl) ((_hctl)->tx_info.tx_id_threshold)
#define SSV6XXX_PAGE_TX_THRESHOLD(_hctl) ((_hctl)->tx_info.tx_page_threshold)
#define SSV6XXX_TX_LOWTHRESHOLD_ID_TRIGGER(_hctl) ((_hctl)->tx_info.tx_lowthreshold_id_trigger)
#define SSV6XXX_TX_LOWTHRESHOLD_PAGE_TRIGGER(_hctl) ((_hctl)->tx_info.tx_lowthreshold_page_trigger)
#define SSV6XXX_ID_AC_BK_OUT_QUEUE(_hctl) ((_hctl)->tx_info.bk_txq_size)
#define SSV6XXX_ID_AC_BE_OUT_QUEUE(_hctl) ((_hctl)->tx_info.be_txq_size)
#define SSV6XXX_ID_AC_VI_OUT_QUEUE(_hctl) ((_hctl)->tx_info.vi_txq_size)
#define SSV6XXX_ID_AC_VO_OUT_QUEUE(_hctl) ((_hctl)->tx_info.vo_txq_size)
#define SSV6XXX_ID_MANAGER_QUEUE(_hctl) ((_hctl)->tx_info.manage_txq_size)
#define SSV6XXX_ID_USB_AC_BK_OUT_QUEUE(_hctl) ((_hctl)->shi->sh->cfg.bk_txq_size)
#define SSV6XXX_ID_USB_AC_BE_OUT_QUEUE(_hctl) ((_hctl)->shi->sh->cfg.be_txq_size)
#define SSV6XXX_ID_USB_AC_VI_OUT_QUEUE(_hctl) ((_hctl)->shi->sh->cfg.vi_txq_size)
#define SSV6XXX_ID_USB_AC_VO_OUT_QUEUE(_hctl) ((_hctl)->shi->sh->cfg.vo_txq_size)
#define SSV6XXX_ID_USB_MANAGER_QUEUE(_hctl) ((_hctl)->shi->sh->cfg.manage_txq_size)
#define SSV6XXX_ID_HCI_INPUT_QUEUE 8
#define HCI_FLAGS_ENQUEUE_HEAD 0x00000001
#define HCI_FLAGS_NO_FLOWCTRL 0x00000002
#define HCI_DBG_PRINT(_hci_ctrl,fmt,...) \
    do { \
        (_hci_ctrl)->shi->dbgprint((_hci_ctrl)->shi->sc, LOG_HCI, fmt, ##__VA_ARGS__); \
    } while (0)
struct ssv_hw_txq {
    u32 txq_no;
    struct sk_buff_head qhead;
    int max_qsize;
    int resum_thres;
    bool paused;
    u32 tx_pkt;
    u32 tx_flags;
};
struct ssv6xxx_hci_ctrl;
struct ssv6xxx_hci_ops {
    int (*hci_start)(struct ssv6xxx_hci_ctrl *hctrl);
    int (*hci_stop)(struct ssv6xxx_hci_ctrl *hctrl);
    void (*hci_write_hw_config)(struct ssv6xxx_hci_ctrl *hctrl, int val);
    int (*hci_read_word)(struct ssv6xxx_hci_ctrl *hctrl, u32 addr, u32 *regval);
    int (*hci_write_word)(struct ssv6xxx_hci_ctrl *hctrl, u32 addr, u32 regval);
    int (*hci_safe_read_word)(struct ssv6xxx_hci_ctrl *hctrl, u32 addr, u32 *regval);
    int (*hci_safe_write_word)(struct ssv6xxx_hci_ctrl *hctrl, u32 addr, u32 regval);
    int (*hci_burst_read_word)(struct ssv6xxx_hci_ctrl *hctrl, u32 *addr, u32 *regval, u8 reg_amount);
    int (*hci_burst_write_word)(struct ssv6xxx_hci_ctrl *hctrl, u32 *addr, u32 *regval, u8 reg_amount);
    int (*hci_burst_safe_read_word)(struct ssv6xxx_hci_ctrl *hctrl, u32 *addr, u32 *regval, u8 reg_amount);
    int (*hci_burst_safe_write_word)(struct ssv6xxx_hci_ctrl *hctrl, u32 *addr, u32 *regval, u8 reg_amount);
    int (*hci_load_fw)(struct ssv6xxx_hci_ctrl *hctrl, u8 *firmware_name, u8 openfile);
    int (*hci_tx)(struct ssv6xxx_hci_ctrl *hctrl, struct sk_buff *, int, u32);
#if 0
    int (*hci_rx)(struct ssv6xxx_hci_ctrl *hctrl, struct sk_buff *);
#endif
    int (*hci_tx_pause)(struct ssv6xxx_hci_ctrl *hctrl, u32 txq_mask);
    int (*hci_tx_resume)(struct ssv6xxx_hci_ctrl *hctrl, u32 txq_mask);
    int (*hci_txq_flush)(struct ssv6xxx_hci_ctrl *hctrl, u32 txq_mask);
    int (*hci_txq_flush_by_sta)(struct ssv6xxx_hci_ctrl *hctrl, int aid);
    bool (*hci_txq_empty)(struct ssv6xxx_hci_ctrl *hctrl, int txqid);
    int (*hci_pmu_wakeup)(struct ssv6xxx_hci_ctrl *hctrl);
    int (*hci_send_cmd)(struct ssv6xxx_hci_ctrl *hctrl, struct sk_buff *);
#ifdef CONFIG_SSV6XXX_DEBUGFS
    bool (*hci_init_debugfs)(struct ssv6xxx_hci_ctrl *hctrl, struct dentry *dev_deugfs_dir);
    void (*hci_deinit_debugfs)(struct ssv6xxx_hci_ctrl *hctrl);
#endif
    int (*hci_write_sram)(struct ssv6xxx_hci_ctrl *hctrl, u32 addr, u8* data, u32 size);
    int (*hci_interface_reset)(struct ssv6xxx_hci_ctrl *hctrl);
    int (*hci_sysplf_reset)(struct ssv6xxx_hci_ctrl *hctrl, u32 addr, u32 value);
};
struct ssv6xxx_hci_info {
    struct device *dev;
    struct ssv6xxx_hwif_ops *if_ops;
    struct ssv6xxx_hci_ops *hci_ops;
    struct ssv6xxx_hci_ctrl *hci_ctrl;
    struct ssv_softc *sc;
    struct ssv_hw *sh;
    #if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
    int (*hci_rx_cb)(struct sk_buff_head *, void *);
    #else
    int (*hci_rx_cb)(struct sk_buff *, void *);
    #endif
    int (*hci_is_rx_q_full)(void *);
    void (*hci_pre_tx_cb)(struct sk_buff *, void *);
    void (*hci_post_tx_cb)(struct sk_buff_head *, void *);
    int (*hci_tx_flow_ctrl_cb)(void *, int, bool, int debug);
    void (*hci_tx_buf_free_cb)(struct sk_buff *, void *);
    void (*hci_skb_update_cb)(struct sk_buff *, void *);
    void (*hci_tx_q_empty_cb)(u32 txq_no, void *);
    int (*hci_rx_mode_cb)(void *);
    int (*hci_peek_next_pkt_len_cb)(struct sk_buff *, void *);
 void (*dbgprint)(void *, u32 log_id, const char *fmt,...);
    struct sk_buff *(*skb_alloc) (void *app_param, s32 len);
    void (*skb_free) (void *app_param, struct sk_buff *skb);
    void (*write_hw_config_cb)(void *param, u32 addr, u32 value);
};
int tu_ssv6xxx_hci_deregister(struct ssv6xxx_hci_info *);
int tu_ssv6xxx_hci_register(struct ssv6xxx_hci_info *);
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
int tu_ssv6xxx_hci_init(void);
void tu_ssv6xxx_hci_exit(void);
#endif
#ifdef SSV_SUPPORT_HAL
#define SSV_READRG_HCI_INQ_INFO(_hci_ctrl,_used_id,_tx_use_page) \
                                                      HAL_READRG_HCI_INQ_INFO((_hci_ctrl)->shi->sh, _used_id, _tx_use_page)
#define SSV_LOAD_FW_ENABLE_MCU(_hci_ctrl) HAL_LOAD_FW_ENABLE_MCU((_hci_ctrl)->shi->sh)
#define SSV_LOAD_FW_DISABLE_MCU(_hci_ctrl) HAL_LOAD_FW_DISABLE_MCU((_hci_ctrl)->shi->sh)
#define SSV_LOAD_FW_SET_STATUS(_hci_ctrl,_status) HAL_LOAD_FW_SET_STATUS((_hci_ctrl)->shi->sh, (_status))
#define SSV_LOAD_FW_GET_STATUS(_hci_ctrl,_status) HAL_LOAD_FW_GET_STATUS((_hci_ctrl)->shi->sh, (_status))
#define SSV_RESET_CPU(_hci_ctrl) HAL_RESET_CPU((_hci_ctrl)->shi->sh)
#define SSV_SET_SRAM_MODE(_hci_ctrl,_mode) HAL_SET_SRAM_MODE((_hci_ctrl)->shi->sh, _mode)
#define SSV_LOAD_FW_PRE_CONFIG_DEVICE(_hci_ctrl) HAL_LOAD_FW_PRE_CONFIG_DEVICE((_hci_ctrl)->shi->sh)
#define SSV_LOAD_FW_POST_CONFIG_DEVICE(_hci_ctrl) HAL_LOAD_FW_POST_CONFIG_DEVICE((_hci_ctrl)->shi->sh)
#else
void ssv6xxx_hci_hci_inq_info(struct ssv6xxx_hci_ctrl *ctrl_hci, int *used_id);
void ssv6xxx_hci_load_fw_enable_mcu(struct ssv6xxx_hci_ctrl *ctrl_hci);
int ssv6xxx_hci_load_fw_disable_mcu(struct ssv6xxx_hci_ctrl *ctrl_hci);
int ssv6xxx_hci_load_fw_set_status(struct ssv6xxx_hci_ctrl *ctrl_hci, int status);
int ssv6xxx_hci_load_fw_get_status(struct ssv6xxx_hci_ctrl *ctrl_hci, int *status);
void ssv6xxx_hci_load_fw_pre_config_device(struct ssv6xxx_hci_ctrl *ctrl_hci);
void ssv6xxx_hci_load_fw_post_config_device(struct ssv6xxx_hci_ctrl *ctrl_hci);
#define SSV_READRG_HCI_INQ_INFO(_hci_ctrl,_used_id,_tx_use_page) \
                                                       ssv6xxx_hci_hci_inq_info(_hci_ctrl, _used_id)
#define SSV_LOAD_FW_ENABLE_MCU(_hci_ctrl) ssv6xxx_hci_load_fw_enable_mcu((_hci_ctrl))
#define SSV_LOAD_FW_DISABLE_MCU(_hci_ctrl) ssv6xxx_hci_load_fw_disable_mcu((_hci_ctrl))
#define SSV_LOAD_FW_SET_STATUS(_hci_ctrl,_status) ssv6xxx_hci_load_fw_set_status((_hci_ctrl), (_status))
#define SSV_LOAD_FW_GET_STATUS(_hci_ctrl,_status) ssv6xxx_hci_load_fw_get_status((_hci_ctrl), (_status))
#define SSV_RESET_CPU(_hci_ctrl) ssv6xxx_hci_reset_cpu((_hci_ctrl))
#define SSV_SET_SRAM_MODE(_hci_ctrl,_mode) 
#define SSV_LOAD_FW_PRE_CONFIG_DEVICE(_hci_ctrl) ssv6xxx_hci_load_fw_pre_config_device(_hci_ctrl)
#define SSV_LOAD_FW_POST_CONFIG_DEVICE(_hci_ctrl) ssv6xxx_hci_load_fw_post_config_device(_hci_ctrl)
#endif
#endif
