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

#ifndef _SSV_CMD_H_
#define _SSV_CMD_H_ 
#define CLI_ARG_SIZE 10
#define PROC_DIR_ENTRY "tu_ssv"
#define PROC_DEVICETYPE_ENTRY "ssv_devicetype"
#define PROC_SSV_CMD_ENTRY "ssv_cmd"
#define PROC_SSV_DBG_ENTRY "ssv_dbg_fs"
#define PROC_SSV_FREQ_ENTRY "freq"
#define PROC_SSV_P2P_ENTRY "p2p"
#define MAX_CHARS_PER_LINE 256
#ifdef CONFIG_SMART_ICOMM
#define PROC_SI_ENTRY "smart_config"
#define PROC_SI_SSID_ENTRY "si_ssid"
#define PROC_SI_PASS_ENTRY "si_pass"
#endif
struct ssv_softc;
struct ssv_cmd_table {
    const char *cmd;
    int (*cmd_func_ptr)(struct ssv_softc *sc, int, char **);
    const char *usage;
    const int result_buffer_size;
};
struct ssv6xxx_cfg_cmd_table {
    u8 *cfg_cmd;
    void *var;
    u32 arg;
    int (*translate_func)(u8 *, void *, u32);
    u8 *def_val;
};
#define SSV_REG_READ1(ops,reg,val) \
        (ops)->ifops->readreg((ops)->dev, reg, val)
#define SSV_REG_WRITE1(ops,reg,val) \
        (ops)->ifops->writereg((ops)->dev, reg, val)
#define SSV_REG_SET_BITS1(ops,reg,set,clr) \
    { \
        u32 reg_val; \
        SSV_REG_READ(ops, reg, &reg_val); \
        reg_val &= ~(clr); \
        reg_val |= (set); \
        SSV_REG_WRITE(ops, reg, reg_val); \
    }
struct ssv_cmd_data;
int ssv_cmd_submit(struct ssv_cmd_data *cmd_data, char *cmd);
void snprintf_res(struct ssv_cmd_data *cmd_data, const char *fmt, ... );
struct sk_buff *ssvdevice_skb_alloc(s32 len);
void ssvdevice_skb_free(struct sk_buff *skb);
#ifdef SSV_SUPPORT_HAL
#define SSV_DUMP_WSID(_sh) HAL_DUMP_WSID(_sh)
#define SSV_DUMP_DECISION(_sh) HAL_DUMP_DECISION(_sh)
#define SSV_READ_FFOUT_CNT(_sh,_value,_value1,_value2) \
                HAL_READ_FFOUT_CNT(_sh, _value, _value1, _value2)
#define SSV_READ_IN_FFCNT(_sh,_value,_value1) HAL_READ_IN_FFCNT(_sh, _value, _value1)
#define SSV_READ_ID_LEN_THRESHOLD(_sh,_tx_len,_rx_len) \
    HAL_READ_ID_LEN_THRESHOLD(_sh, _tx_len, _rx_len)
#define SSV_READ_TAG_STATUS(_sh,_ava_status) HAL_READ_TAG_STATUS(_sh, _ava_status)
#define SSV_CMD_MIB(_sc,_argc,_argv) HAL_CMD_MIB(_sc, _argc, _argv)
#define SSV_CMD_POWER_SAVING(_sc,_argc,_argv) HAL_CMD_POWER_SAVING(_sc, _argc, _argv)
#define SSV_GET_FW_VERSION(_sh,_regval) HAL_GET_FW_VERSION(_sh, _regval)
#define SSV_TXTPUT_SET_DESC(_sh,_skb) HAL_TXTPUT_SET_DESC(_sh, _skb)
#define SSV_READRG_TXQ_INFO2(_ifops,_sh,_txq_info2) \
                HAL_READRG_TXQ_INFO2(_sh , _txq_info2)
#define SSV_GET_RD_ID_ADR(_sh,_id_base_addr) HAL_GET_RD_ID_ADR(_sh, _id_base_addr)
#define SSV_GET_FFOUT_CNT(_sh,_value,_tag) HAL_GET_FFOUT_CNT(_sh, _value, _tag)
#define SSV_GET_IN_FFCNT(_sh,_value,_tag) HAL_GET_IN_FFCNT(_sh, _value, _tag)
#define SSV_AUTO_GEN_NULLPKT(_sh,_hwq) HAL_AUTO_GEN_NULLPKT(_sh, _hwq)
#else
#define SSV_DUMP_WSID(_sh) ssv6xxx_dump_wsid(_sh)
#define SSV_DUMP_DECISION(_sh) ssv6xxx_dump_decision(_sh)
#define SSV_READ_FFOUT_CNT(_sh,_value,_value1,_value2) \
                ssv6xxx_read_ffout_cnt(_sh, _value, _value1, _value2)
#define SSV_READ_IN_FFCNT(_sh,_value,_value1) ssv6xxx_read_in_ffcnt(_sh, _value, _value1)
#define SSV_READ_ID_LEN_THRESHOLD(_sh,_tx_len,_rx_len) \
    ssv6xxx_read_id_len_threshold(_sh, _tx_len, _rx_len)
#define SSV_READ_TAG_STATUS(_sh,_ava_status) ssv6xxx_read_tag_status(_sh, _ava_status)
#define SSV_CMD_MIB(_sh,_argc,argv) ssv6xxx_cmd_mib(_sh, _argc, argv)
#define SSV_CMD_POWER_SAVING(_sh,_argc,argv) ssv6xxx_cmd_power_saving(_sh, _argc, argv)
#define SSV_GET_FW_VERSION(_sh,_regval) ssv6xxx_get_fw_version(_sh, _regval)
#define SSV_TXTPUT_SET_DESC(_sh,_skb) ssv6xxx_txtput_set_desc(_sh, _skb)
#define SSV_READRG_TXQ_INFO2(_ifops,_sh,_txq_info2) \
            SSV_REG_READ1(_ifops, ADR_TX_ID_ALL_INFO2, _txq_info2)
#define SSV_GET_RD_ID_ADR(_sh,_id_base_addr) ssv6xxx_get_rd_id_adr(_id_base_addr)
#define SSV_GET_FFOUT_CNT(_sh,_value,_tag) ssv6xxx_get_ffout_cnt(_value, _tag)
#define SSV_GET_IN_FFCNT(_sh,_value,_tag) ssv6xxx_get_in_ffcnt(_value, _tag)
#define SSV_AUTO_GEN_NULLPKT(_sh,_hwq) ssv6xxx_auto_gen_nullpkt(_sh, _hwq)
#endif
#endif
