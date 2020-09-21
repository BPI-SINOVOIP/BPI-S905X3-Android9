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

#include <linux/kernel.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
#include <linux/export.h>
#else
#include <linux/module.h>
#endif
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/sched.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0)
#include <linux/sched/prio.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0)
#include <linux/sched/rt.h>
#endif
#include <ssv_conf_parser.h>
#ifndef SSV_SUPPORT_HAL
#include <ssv6200_reg.h>
#endif
#include <ssv6200.h>
#include <hci/hctrl.h>
#include <smac/dev.h>
#include <hal.h>
#include "ssv_cmd.h"
#include <ssv_version.h>
#include <linux_80211.h>
#ifndef CONFIG_SSV_CABRIO_A
#include <ssv6200_configuration.h>
#endif
#define SSV_CMD_PRINTF() 
struct ssv6xxx_dev_table {
    u32 address;
    u32 val;
};
EXPORT_SYMBOL(snprintf_res);
#ifndef SSV_SUPPORT_HAL
static bool ssv6xxx_dump_wsid(struct ssv_hw *sh)
{
    const u32 reg_wsid[]={ ADR_WSID0, ADR_WSID1 };
 const u32 reg_wsid_tid0[]={ ADR_WSID0_TID0_RX_SEQ, ADR_WSID1_TID0_RX_SEQ };
 const u32 reg_wsid_tid7[]={ ADR_WSID0_TID7_RX_SEQ, ADR_WSID1_TID7_RX_SEQ };
    const u8 *op_mode_str[]={"STA", "AP", "AD-HOC", "WDS"};
    const u8 *ht_mode_str[]={"Non-HT", "HT-MF", "HT-GF", "RSVD"};
    u32 addr, regval;
    int s;
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    for (s = 0; s < SSV_NUM_HW_STA; s++) {
        if(SMAC_REG_READ(sh, reg_wsid[s], &regval));
        snprintf_res(cmd_data, "==>WSID[%d]\n\tvalid[%d] qos[%d] op_mode[%s] ht_mode[%s]\n",
            s, regval&0x1, (regval>>1)&0x1, op_mode_str[((regval>>2)&3)], ht_mode_str[((regval>>4)&3)]);
        if(SMAC_REG_READ(sh, reg_wsid[s]+4, &regval));
        snprintf_res(cmd_data, "\tMAC[%02x:%02x:%02x:%02x:",
               (regval&0xff), ((regval>>8)&0xff), ((regval>>16)&0xff), ((regval>>24)&0xff));
        if(SMAC_REG_READ(sh, reg_wsid[s]+8, &regval));
        snprintf_res(cmd_data, "%02x:%02x]\n",
               (regval&0xff), ((regval>>8)&0xff));
        for(addr = reg_wsid_tid0[s]; addr <= reg_wsid_tid7[s]; addr+=4){
            if(SMAC_REG_READ(sh, addr, &regval));
            snprintf_res(cmd_data, "\trx_seq%d[%d]\n", ((addr-reg_wsid_tid0[s])>>2), ((regval)&0xffff));
        }
    }
    return 0;
}
static bool ssv6xxx_dump_decision(struct ssv_hw *sh)
{
    u32 addr, regval;
    int s;
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    snprintf_res(cmd_data, ">> Decision Table:\n");
    for(s = 0, addr = ADR_MRX_FLT_TB0; s < 16; s++, addr+=4) {
        if(SMAC_REG_READ(sh, addr, &regval));
        snprintf_res(cmd_data, "   [%d]: ADDR[0x%08x] = 0x%08x\n",
            s, addr, regval);
    }
    snprintf_res(cmd_data, "\n\n>> Decision Mask:\n");
    for (s = 0, addr = ADR_MRX_FLT_EN0; s < 9; s++, addr+=4) {
        if(SMAC_REG_READ(sh, addr, &regval));
        snprintf_res(cmd_data, "   [%d]: ADDR[0x%08x] = 0x%08x\n",
            s, addr, regval);
    }
    snprintf_res(cmd_data, "\n\n");
    return 0;
}
static u32 ssv6xxx_get_ffout_cnt(u32 value, int tag)
{
 switch (tag) {
  case M_ENG_CPU:
   return ((value & FFO0_CNT_MSK) >> FFO0_CNT_SFT);
  case M_ENG_HWHCI:
   return ((value & FFO1_CNT_MSK) >> FFO1_CNT_SFT);
  case M_ENG_ENCRYPT:
   return ((value & FFO3_CNT_MSK) >> FFO3_CNT_SFT);
  case M_ENG_MACRX:
   return ((value & FFO4_CNT_MSK) >> FFO4_CNT_SFT);
  case M_ENG_MIC:
   return ((value & FFO5_CNT_MSK) >> FFO5_CNT_SFT);
  case M_ENG_TX_EDCA0:
   return ((value & FFO6_CNT_MSK) >> FFO6_CNT_SFT);
  case M_ENG_TX_EDCA1:
   return ((value & FFO7_CNT_MSK) >> FFO7_CNT_SFT);
  case M_ENG_TX_EDCA2:
   return ((value & FFO8_CNT_MSK) >> FFO8_CNT_SFT);
  case M_ENG_TX_EDCA3:
   return ((value & FFO9_CNT_MSK) >> FFO9_CNT_SFT);
  case M_ENG_TX_MNG:
   return ((value & FFO10_CNT_MSK) >> FFO10_CNT_SFT);
  case M_ENG_ENCRYPT_SEC:
   return ((value & FFO11_CNT_MSK) >> FFO11_CNT_SFT);
  case M_ENG_MIC_SEC:
   return ((value & FFO12_CNT_MSK) >> FFO12_CNT_SFT);
  case M_ENG_TRASH_CAN:
   return ((value & FFO15_CNT_MSK) >> FFO15_CNT_SFT);
  default:
   return 0;
 }
}
static u32 ssv6xxx_get_in_ffcnt(u32 value, int tag)
{
 switch (tag) {
  case M_ENG_CPU:
   return ((value & FF0_CNT_MSK) >> FF0_CNT_SFT);
  case M_ENG_HWHCI:
   return ((value & FF1_CNT_MSK) >> FF1_CNT_SFT);
  case M_ENG_ENCRYPT:
   return ((value & FF3_CNT_MSK) >> FF3_CNT_SFT);
  case M_ENG_MACRX:
   return ((value & FF4_CNT_MSK) >> FF4_CNT_SFT);
  case M_ENG_MIC:
   return ((value & FF5_CNT_MSK) >> FF5_CNT_SFT);
  case M_ENG_TX_EDCA0:
   return ((value & FF6_CNT_MSK) >> FF6_CNT_SFT);
  case M_ENG_TX_EDCA1:
   return ((value & FF7_CNT_MSK) >> FF7_CNT_SFT);
  case M_ENG_TX_EDCA2:
   return ((value & FF8_CNT_MSK) >> FF8_CNT_SFT);
  case M_ENG_TX_EDCA3:
   return ((value & FF9_CNT_MSK) >> FF9_CNT_SFT);
  case M_ENG_TX_MNG:
   return ((value & FF10_CNT_MSK) >> FF10_CNT_SFT);
  case M_ENG_ENCRYPT_SEC:
   return ((value & FF11_CNT_MSK) >> FF11_CNT_SFT);
  case M_ENG_MIC_SEC:
   return ((value & FF12_CNT_MSK) >> FF12_CNT_SFT);
  case M_ENG_TRASH_CAN:
   return ((value & FF15_CNT_MSK) >> FF15_CNT_SFT);
  default:
   return 0;
 }
}
static void ssv6xxx_read_ffout_cnt(struct ssv_hw *sh,
    u32 *value, u32 *value1, u32 *value2)
{
    if(SMAC_REG_READ(sh, ADR_RD_FFOUT_CNT1, value));
    if(SMAC_REG_READ(sh, ADR_RD_FFOUT_CNT2, value1));
    if(SMAC_REG_READ(sh, ADR_RD_FFOUT_CNT3, value2));
}
static void ssv6xxx_read_in_ffcnt(struct ssv_hw *sh,
    u32 *value, u32 *value1)
{
    if(SMAC_REG_READ(sh, ADR_RD_IN_FFCNT1, value));
    if(SMAC_REG_READ(sh, ADR_RD_IN_FFCNT2, value1));
}
static void ssv6xxx_read_id_len_threshold(struct ssv_hw *sh,
    u32 *tx_len, u32 *rx_len)
{
 u32 regval = 0;
    if(SMAC_REG_READ(sh, ADR_ID_LEN_THREADSHOLD2, &regval));
 *tx_len = ((regval & TX_ID_ALC_LEN_MSK) >> TX_ID_ALC_LEN_SFT);
 *rx_len = ((regval & RX_ID_ALC_LEN_MSK) >> RX_ID_ALC_LEN_SFT);
}
static void ssv6xxx_read_tag_status(struct ssv_hw *sh,
    u32 *ava_status)
{
 u32 regval = 0;
 if(SMAC_REG_READ(sh, ADR_TAG_STATUS, &regval));
 *ava_status = ((regval & AVA_TAG_MSK) >> AVA_TAG_SFT);
}
static void ssv6xxx_dump_mib_rx_phy(struct ssv_hw *sh)
{
    u32 value;
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    snprintf_res(cmd_data, "PHY B mode:\n");
    snprintf_res(cmd_data, "%-10s\t\t%-10s\t\t%-10s\n", "CRC error","CCA","counter");
    if(SMAC_REG_READ(sh, ADR_RX_11B_PKT_ERR_AND_PKT_ERR_CNT, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", value & B_PACKET_ERR_CNT_MSK);
    if(SMAC_REG_READ(sh, ADR_RX_11B_PKT_CCA_AND_PKT_CNT, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", (value & B_CCA_CNT_MSK) >> B_CCA_CNT_SFT);
    snprintf_res(cmd_data, "[%08x]\t\t\n\n", value & B_PACKET_CNT_MSK);
    snprintf_res(cmd_data, "PHY G/N mode:\n");
    snprintf_res(cmd_data, "%-10s\t\t%-10s\t\t%-10s\n", "CRC error","CCA","counter");
    if(SMAC_REG_READ(sh, ADR_RX_11GN_PKT_ERR_CNT, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", value & GN_PACKET_ERR_CNT_MSK);
    if(SMAC_REG_READ(sh, ADR_RX_11GN_PKT_CCA_AND_PKT_CNT, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", (value & GN_CCA_CNT_MSK) >> GN_CCA_CNT_SFT);
    snprintf_res(cmd_data, "[%08x]\t\t\n\n", value & GN_PACKET_CNT_MSK);
}
static void ssv6xxx_reset_mib_phy(struct ssv_hw *sh)
{
    if(SMAC_REG_WRITE(sh, ADR_RX_11B_PKT_STAT_EN, 0));
    msleep(1);
    if(SMAC_REG_WRITE(sh, ADR_RX_11B_PKT_STAT_EN, RG_PACKET_STAT_EN_11B_MSK));
    if(SMAC_REG_WRITE(sh, ADR_RX_11GN_STAT_EN, 0));
    msleep(1);
    if(SMAC_REG_WRITE(sh, ADR_RX_11GN_STAT_EN, RG_PACKET_STAT_EN_11GN_MSK));
    if(SMAC_REG_WRITE(sh, ADR_PHY_REG_20_MRX_CNT, 0));
    msleep(1);
    if(SMAC_REG_WRITE(sh, ADR_PHY_REG_20_MRX_CNT, RG_MRX_EN_CNT_RST_N_MSK));
}
static void ssv6xxx_reset_mib(struct ssv_hw *sh)
{
    if(SMAC_REG_WRITE(sh, ADR_MIB_EN, 0));
    msleep(1);
    if(SMAC_REG_WRITE(sh, ADR_MIB_EN, 0xffffffff));
    ssv6xxx_reset_mib_phy(sh);
}
static void ssv6xxx_list_mib(struct ssv_hw *sh)
{
    u32 addr, value;
    int i;
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    addr = MIB_REG_BASE;
    for (i = 0; i < 120; i++, addr+=4) {
        if(SMAC_REG_READ(sh, addr, &value));
        snprintf_res(cmd_data, "%08x ", value);
        if (((i+1) & 0x07) == 0)
            snprintf_res(cmd_data, "\n");
    }
    snprintf_res(cmd_data, "\n");
}
static void ssv6xxx_dump_mib_rx(struct ssv_hw *sh)
{
    u32 value;
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    snprintf_res(cmd_data, "%-10s\t\t%-10s\t\t%-10s\t\t%-10s\n",
        "MRX_FCS_SUCC", "MRX_FCS_ERR", "MRX_ALC_FAIL", "MRX_MISS");
    if(SMAC_REG_READ(sh, ADR_MRX_FCS_SUCC, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", value);
    if(SMAC_REG_READ(sh, ADR_MRX_FCS_ERR, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", value);
    if(SMAC_REG_READ(sh, ADR_MRX_ALC_FAIL, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", value);
    if(SMAC_REG_READ(sh, ADR_MRX_MISS, &value));
    snprintf_res(cmd_data, "[%08x]\n", value);
    snprintf_res(cmd_data, "%-10s\t\t%-10s\t\t%-10s\t%-10s\n",
        "MRX_MB_MISS", "MRX_NIDLE_MISS", "DBG_LEN_ALC_FAIL", "DBG_LEN_CRC_FAIL");
    if(SMAC_REG_READ(sh, ADR_MRX_MB_MISS, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", value);
    if(SMAC_REG_READ(sh, ADR_MRX_NIDLE_MISS, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", value);
    if(SMAC_REG_READ(sh, ADR_DBG_LEN_ALC_FAIL, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", value);
    if(SMAC_REG_READ(sh, ADR_DBG_LEN_CRC_FAIL, &value));
    snprintf_res(cmd_data, "[%08x]\n\n", value);
    snprintf_res(cmd_data, "%-10s\t\t%-10s\t\t%-10s\t%-10s\n",
        "DBG_AMPDU_PASS", "DBG_AMPDU_FAIL", "ID_ALC_FAIL1", "ID_ALC_FAIL2");
    if(SMAC_REG_READ(sh, ADR_DBG_AMPDU_PASS, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", value);
    if(SMAC_REG_READ(sh, ADR_DBG_AMPDU_FAIL, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", value);
    if(SMAC_REG_READ(sh, ADR_ID_ALC_FAIL1, &value));
    snprintf_res(cmd_data, "[%08x]\t\t", value);
    if(SMAC_REG_READ(sh, ADR_ID_ALC_FAIL2, &value));
    snprintf_res(cmd_data, "[%08x]\n\n", value);
    ssv6xxx_dump_mib_rx_phy(sh);
}
static void ssv6xxx_cmd_mib(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    if ((argc == 2) && (!strcmp(argv[1], "reset"))) {
        ssv6xxx_reset_mib(sc->sh);
        snprintf_res(cmd_data, " => MIB reseted\n");
    } else if ((argc == 2) && (!strcmp(argv[1], "list"))) {
        ssv6xxx_list_mib(sc->sh);
    } else if ((argc == 2) && (strcmp(argv[1], "rx") == 0)) {
        ssv6xxx_dump_mib_rx(sc->sh);
    } else {
        snprintf_res(cmd_data, "mib [reset|list|rx]\n\n");
    }
}
static void ssv6xxx_cmd_power_saving(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    char *endp;
    if ((argc == 2) && (argv[1])) {
        sc->ps_aid = simple_strtoul(argv[1], &endp, 10);
    } else {
        snprintf_res(cmd_data, "ps [aid_value]\n\n");
    }
    ssv6xxx_trigger_pmu(sc);
}
static void ssv6xxx_get_rd_id_adr(u32 *id_base_address)
{
    id_base_address[0] = ADR_RD_ID0;
    id_base_address[1] = ADR_RD_ID1;
    id_base_address[2] = ADR_RD_ID2;
    id_base_address[3] = ADR_RD_ID3;
}
static void ssv6xxx_txtput_set_desc(struct ssv_hw *sh, struct sk_buff *skb )
{
    struct ssv6200_tx_desc *tx_desc;
 tx_desc = (struct ssv6200_tx_desc *)skb->data;
 memset((void *)tx_desc, 0xff, sizeof(struct ssv6200_tx_desc));
 tx_desc->len = skb->len;
 tx_desc->c_type = M2_TXREQ;
 tx_desc->fCmd = (M_ENG_CPU << 4) | M_ENG_HWHCI;
 tx_desc->reason = ID_TRAP_SW_TXTPUT;
}
static void ssv6xxx_get_fw_version(struct ssv_hw *sh, u32 *regval)
{
    if(SMAC_REG_READ(sh, ADR_TX_SEG, regval));
}
static int ssv6xxx_auto_gen_nullpkt(struct ssv_hw *sh, int hwq)
{
    return -EOPNOTSUPP;
}
#endif
struct sk_buff *ssvdevice_skb_alloc(s32 len)
{
    struct sk_buff *skb;
    skb = __dev_alloc_skb(len + SSV6200_ALLOC_RSVD , GFP_KERNEL);
    if (skb != NULL) {
        skb_put(skb,0x20);
        skb_pull(skb,0x20);
    }
    return skb;
}
void ssvdevice_skb_free(struct sk_buff *skb)
{
    dev_kfree_skb_any(skb);
}
static int ssv_cmd_help(struct ssv_softc *sc, int argc, char *argv[])
{
    extern struct ssv_cmd_table cmd_table[];
    struct ssv_cmd_table *sc_tbl;
    int total_cmd = 0;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    snprintf_res(cmd_data, "Usage:\n");
    for (sc_tbl = &cmd_table[3]; sc_tbl->cmd; sc_tbl++) {
        snprintf_res(cmd_data, "%-20s\t\t%s\n", (char*)sc_tbl->cmd, sc_tbl->usage);
        total_cmd++;
    }
    snprintf_res(cmd_data, "Total CMDs: %x\n\nType cli help [CMD] for more detail command.\n\n", total_cmd);
    return 0;
}
static int ssv_cmd_reg(struct ssv_softc *sc, int argc, char *argv[])
{
    u32 addr, value, count;
    char *endp;
    int s;
#ifdef SSV_SUPPORT_HAL
    int ret = 0;
#endif
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    if ((argc == 4) && (strcmp(argv[1], "w") == 0)) {
        addr = simple_strtoul(argv[2], &endp, 16);
        value = simple_strtoul(argv[3], &endp, 16);
        if (SMAC_REG_WRITE(sc->sh, addr, value)) ;
        snprintf_res(cmd_data, " => write [0x%08x]: 0x%08x\n", addr, value);
        return 0;
    } else if (((argc == 4) || (argc == 3)) && (strcmp(argv[1], "r") == 0)) {
        count = (argc ==3 )? 1: simple_strtoul(argv[3], &endp, 10);
        addr = simple_strtoul(argv[2], &endp, 16);
        snprintf_res(cmd_data, "ADDRESS: 0x%08x\n", addr);
        for(s = 0; s < count; s++, addr += 4) {
            if (SMAC_REG_READ(sc->sh, addr, &value));
            snprintf_res(cmd_data, "%08x ", value);
            if (((s+1) & 0x07) == 0){
                snprintf_res(cmd_data, "\n");
            }
        }
        snprintf_res(cmd_data, "\n");
        return 0;
    }
#ifdef SSV_SUPPORT_HAL
    else if (argc == 5 && strcmp(argv[1], "bw")==0) {
        u32 addr_list[8],value_list[8];
        addr = simple_strtoul(argv[2], &endp, 16);
        value = simple_strtoul(argv[3], &endp, 16);
        count = simple_strtoul(argv[4], &endp, 16);
        for (s=0; s<count; s++) {
            addr_list[s] = addr+4*s;
            value_list[s] = value;
        }
        ret = HAL_BURST_WRITE_REG(sc->sh, addr_list, value_list, count);
        if (ret >= 0) {
            snprintf_res(cmd_data, "  ==> write done.\n");
            return 0;
        } else if (ret == -EOPNOTSUPP) {
            snprintf_res(cmd_data, "Does not support this command!\n");
            return 0;
        }
    } else if (argc == 4 && strcmp(argv[1], "br")==0) {
        u32 addr_list[8],value_list[8];
        addr = simple_strtoul(argv[2], &endp, 16);
        count = simple_strtoul(argv[3], &endp, 16);
        for (s=0; s<count; s++)
            addr_list[s] = addr+4*s;
        ret = HAL_BURST_READ_REG(sc->sh, addr_list, value_list, count);
        if (ret >= 0) {
            snprintf_res(cmd_data, "ADDRESS:   0x%x\n", addr);
            snprintf_res(cmd_data, "REG-COUNT: %d\n", count);
            for (s=0; s<count; s++)
                snprintf_res(cmd_data, "addr %x ==> %x\n", addr_list[s], value_list[s]);
            return 0;
        } else if (ret == -EOPNOTSUPP) {
            snprintf_res(cmd_data, "Does not support this command!\n");
            return 0;
        }
    }
#endif
 else {
        snprintf_res(cmd_data, "reg [r|w] [address] [value|word-count]\n\n");
#ifdef SSV_SUPPORT_HAL
        snprintf_res(cmd_data, "reg [br] [address] [word-count]\n\n");
        snprintf_res(cmd_data, "reg [bw] [address] [value] [word-count]\n\n");
#endif
        return 0;
    }
    return -1;
}
struct ssv6xxx_cfg tu_ssv_cfg;
EXPORT_SYMBOL(tu_ssv_cfg);
#if 0
static int __string2s32(u8 *val_str, void *val)
{
    char *endp;
    int base=10;
    if (val_str[0]=='0' && ((val_str[1]=='x')||(val_str[1]=='X')))
        base = 16;
    *(int *)val = simple_strtoul(val_str, &endp, base);
    return 0;
}
#endif
static int __string2bool(u8 *u8str, void *val, u32 arg)
{
    char *endp;
 *(u8 *)val = !!simple_strtoul(u8str, &endp, 10);
    return 0;
}
static int __string2u32(u8 *u8str, void *val, u32 arg)
{
    char *endp;
    int base=10;
    if (u8str[0]=='0' && ((u8str[1]=='x')||(u8str[1]=='X')))
        base = 16;
    *(u32 *)val = simple_strtoul(u8str, &endp, base);
    return 0;
}
static int __string2flag32(u8 *flag_str, void *flag, u32 arg)
{
    u32 *val=(u32 *)flag;
    if (arg >= (sizeof(u32)<<3))
        return -1;
    if (strcmp(flag_str, "on")==0) {
        *val |= (1<<arg);
        return 0;
    }
    if (strcmp(flag_str, "off")==0) {
        *val &= ~(1<<arg);
        return 0;
    }
    return -1;
}
static int __string2mac(u8 *mac_str, void *val, u32 arg)
{
    int s, macaddr[6];
    u8 *mac=(u8 *)val;
    s = sscanf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
        &macaddr[0], &macaddr[1], &macaddr[2],
        &macaddr[3], &macaddr[4], &macaddr[5]);
    if (s != 6)
        return -1;
    mac[0] = (u8)macaddr[0], mac[1] = (u8)macaddr[1];
    mac[2] = (u8)macaddr[2], mac[3] = (u8)macaddr[3];
    mac[4] = (u8)macaddr[4], mac[5] = (u8)macaddr[5];
    return 0;
}
static int __string2str(u8 *path, void *val, u32 arg)
{
    u8 *temp=(u8 *)val;
    sprintf(temp,"%s",path);
    return 0;
}
static int __string2configuration(u8 *mac_str, void *val, u32 arg)
{
    unsigned int address,value;
    int i;
    i = sscanf(mac_str, "%08x:%08x", &address, &value);
    if (i != 2)
        return -1;
    for(i=0; i<EXTERNEL_CONFIG_SUPPORT; i++)
    {
        if(tu_ssv_cfg.configuration[i][0] == 0x0)
        {
            tu_ssv_cfg.configuration[i][0] = address;
            tu_ssv_cfg.configuration[i][1] = value;
            return 0;
        }
    }
    return 0;
}
struct ssv6xxx_cfg_cmd_table tu_cfg_cmds[] = {
    { "hw_mac", (void *)&tu_ssv_cfg.maddr[0][0], 0, __string2mac , NULL},
    { "hw_mac_2", (void *)&tu_ssv_cfg.maddr[1][0], 0, __string2mac , NULL},
    { "def_chan", (void *)&tu_ssv_cfg.def_chan, 0, __string2u32 , "6"},
    { "hw_cap_ht", (void *)&tu_ssv_cfg.hw_caps, 0, __string2flag32 , "on"},
    { "hw_cap_gf", (void *)&tu_ssv_cfg.hw_caps, 1, __string2flag32 , "off"},
    { "hw_cap_2ghz", (void *)&tu_ssv_cfg.hw_caps, 2, __string2flag32 , "on"},
    { "hw_cap_5ghz", (void *)&tu_ssv_cfg.hw_caps, 3, __string2flag32 , "off"},
    { "hw_cap_security", (void *)&tu_ssv_cfg.hw_caps, 4, __string2flag32 , "on"},
    { "hw_cap_sgi", (void *)&tu_ssv_cfg.hw_caps, 5, __string2flag32 , "on"},
    { "hw_cap_ht40", (void *)&tu_ssv_cfg.hw_caps, 6, __string2flag32 , "on"},
    { "hw_cap_ap", (void *)&tu_ssv_cfg.hw_caps, 7, __string2flag32 , "on"},
    { "hw_cap_p2p", (void *)&tu_ssv_cfg.hw_caps, 8, __string2flag32 , "on"},
    { "hw_cap_ampdu_rx", (void *)&tu_ssv_cfg.hw_caps, 9, __string2flag32 , "on"},
    { "hw_cap_ampdu_tx", (void *)&tu_ssv_cfg.hw_caps, 10, __string2flag32 , "on"},
    { "hw_cap_tdls", (void *)&tu_ssv_cfg.hw_caps, 11, __string2flag32 , "off"},
    { "hw_cap_stbc", (void *)&tu_ssv_cfg.hw_caps, 12, __string2flag32 , "on"},
    { "hw_cap_hci_rx_aggr", (void *)&tu_ssv_cfg.hw_caps, 13, __string2flag32 , "on"},
    { "hw_beacon", (void *)&tu_ssv_cfg.hw_caps, 14, __string2flag32 , "on"},
    { "use_wpa2_only", (void *)&tu_ssv_cfg.use_wpa2_only, 0, __string2u32 , NULL},
    { "wifi_tx_gain_level_gn",(void *)&tu_ssv_cfg.wifi_tx_gain_level_gn, 0, __string2u32 , NULL},
    { "wifi_tx_gain_level_b", (void *)&tu_ssv_cfg.wifi_tx_gain_level_b, 0, __string2u32 , NULL},
    { "xtal_clock", (void *)&tu_ssv_cfg.crystal_type, 0, __string2u32 , "24"},
    { "volt_regulator", (void *)&tu_ssv_cfg.volt_regulator, 0, __string2u32 , "0"},
    { "firmware_path", (void *)&tu_ssv_cfg.firmware_path[0], 0, __string2str , NULL},
    { "flash_bin_path", (void *)&tu_ssv_cfg.flash_bin_path[0], 0, __string2str , NULL},
    { "mac_address_path", (void *)&tu_ssv_cfg.mac_address_path[0], 0, __string2str , NULL},
    { "mac_output_path", (void *)&tu_ssv_cfg.mac_output_path[0], 0, __string2str , NULL},
    { "ignore_efuse_mac", (void *)&tu_ssv_cfg.ignore_efuse_mac, 0, __string2u32 , NULL},
    { "efuse_rate_gain_mask", (void *)&tu_ssv_cfg.efuse_rate_gain_mask, 0, __string2u32 , "0x1"},
    { "mac_address_mode", (void *)&tu_ssv_cfg.mac_address_mode, 0, __string2u32 , NULL},
    { "register", NULL, 0, __string2configuration, NULL},
    { "beacon_rssi_minimal", (void *)&tu_ssv_cfg.beacon_rssi_minimal, 0, __string2u32 , NULL},
    { "force_chip_identity", (void *)&tu_ssv_cfg.force_chip_identity, 0, __string2u32 , NULL},
    { "external_firmware_name", (void *)&tu_ssv_cfg.external_firmware_name[0], 0, __string2str, NULL},
    { "ignore_firmware_version", (void *)&tu_ssv_cfg.ignore_firmware_version, 0, __string2u32, NULL},
    { "use_sw_cipher", (void *)&tu_ssv_cfg.use_sw_cipher, 0, __string2u32, NULL},
    { "rc_ht_support_cck", (void *)&tu_ssv_cfg.rc_ht_support_cck, 0, __string2u32 , "1"},
    { "auto_rate_enable", (void *)&tu_ssv_cfg.auto_rate_enable, 0, __string2u32 , "1"},
    { "rc_rate_idx_set", (void *)&tu_ssv_cfg.rc_rate_idx_set, 0, __string2u32 , "0x7777"},
    { "rc_retry_set", (void *)&tu_ssv_cfg.rc_retry_set, 0, __string2u32 , "0x4444"},
    { "rc_mf", (void *)&tu_ssv_cfg.rc_mf, 0, __string2u32 , NULL},
    { "rc_long_short", (void *)&tu_ssv_cfg.rc_long_short, 0, __string2u32 , NULL},
    { "rc_ht40", (void *)&tu_ssv_cfg.rc_ht40, 0, __string2u32 , NULL},
    { "rc_phy_mode" , (void *)&tu_ssv_cfg.rc_phy_mode, 0, __string2u32 , "3"},
 { "tx_id_threshold" , (void *)&tu_ssv_cfg.tx_id_threshold, 0, __string2u32 , NULL},
 { "tx_page_threshold" , (void *)&tu_ssv_cfg.tx_page_threshold, 0, __string2u32 , NULL},
 { "max_rx_aggr_size", (void *)&tu_ssv_cfg.max_rx_aggr_size, 0, __string2u32 , "64"},
 { "online_reset", (void *)&tu_ssv_cfg.online_reset, 0, __string2u32 , "0x00f"},
 { "rx_burstread", (void *)&tu_ssv_cfg.rx_burstread, 0, __string2bool , "0"},
    { "hw_rx_agg_cnt", (void *)&tu_ssv_cfg.hw_rx_agg_cnt, 0, __string2u32 , "3"},
 { "hw_rx_agg_method_3", (void *)&tu_ssv_cfg.hw_rx_agg_method_3, 0, __string2bool , "0"},
 { "hw_rx_agg_timer_reload", (void *)&tu_ssv_cfg.hw_rx_agg_timer_reload, 0, __string2u32 , "20"},
 { "usb_hw_resource", (void *)&tu_ssv_cfg.usb_hw_resource, 0, __string2u32 , "0"},
 { "tx_stuck_detect", (void *)&tu_ssv_cfg.tx_stuck_detect, 0, __string2bool , "0"},
 { "clk_src_80m", (void *)&tu_ssv_cfg.clk_src_80m, 0, __string2bool , "1"},
 { "rts_thres_len", (void *)&tu_ssv_cfg.rts_thres_len, 0, __string2u32 , "0"},
    { "cci", (void *)&tu_ssv_cfg.cci, 0, __string2u32 , "0x19"},
    { "bk_txq_size", (void *)&tu_ssv_cfg.bk_txq_size, 0, __string2u32 , "6"},
    { "be_txq_size", (void *)&tu_ssv_cfg.be_txq_size, 0, __string2u32 , "10"},
    { "vi_txq_size", (void *)&tu_ssv_cfg.vi_txq_size, 0, __string2u32 , "10"},
    { "vo_txq_size", (void *)&tu_ssv_cfg.vo_txq_size, 0, __string2u32 , "8"},
    { "manage_txq_size", (void *)&tu_ssv_cfg.manage_txq_size, 0, __string2u32 , "8"},
    { "aggr_size_sel_pr", (void *)&tu_ssv_cfg.aggr_size_sel_pr, 0, __string2u32 , "95"},
    { "aging_period", (void *)&tu_ssv_cfg.rc_setting.aging_period,0, __string2u32 , "2000"},
    { "target_success_67", (void *)&tu_ssv_cfg.rc_setting.target_success_67, 0, __string2u32 , "55"},
    { "target_success_5", (void *)&tu_ssv_cfg.rc_setting.target_success_5 , 0, __string2u32 , "78"},
    { "target_success_4", (void *)&tu_ssv_cfg.rc_setting.target_success_4 , 0, __string2u32 , "70"},
    { "target_success", (void *)&tu_ssv_cfg.rc_setting.target_success , 0, __string2u32 , "50"},
    { "up_pr", (void *)&tu_ssv_cfg.rc_setting.up_pr, 0, __string2u32 , "70"},
    { "up_pr3", (void *)&tu_ssv_cfg.rc_setting.up_pr3, 0, __string2u32 , "85"},
    { "up_pr4", (void *)&tu_ssv_cfg.rc_setting.up_pr4, 0, __string2u32 , "97"},
    { "up_pr5", (void *)&tu_ssv_cfg.rc_setting.up_pr5, 0, __string2u32 , "70"},
    { "up_pr6", (void *)&tu_ssv_cfg.rc_setting.up_pr6, 0, __string2u32 , "70"},
    { "forbid", (void *)&tu_ssv_cfg.rc_setting.forbid, 0, __string2u32 , "0"},
    { "forbid3", (void *)&tu_ssv_cfg.rc_setting.forbid3, 0, __string2u32 , "0"},
    { "forbid4", (void *)&tu_ssv_cfg.rc_setting.forbid4, 0, __string2u32 , "350"},
    { "forbid5", (void *)&tu_ssv_cfg.rc_setting.forbid5, 0, __string2u32 , "100"},
    { "forbid6", (void *)&tu_ssv_cfg.rc_setting.forbid6, 0, __string2u32 , "0"},
    { "sample_pr_4", (void *)&tu_ssv_cfg.rc_setting.sample_pr_4, 0, __string2u32 , "70"},
    { "sample_pr_5", (void *)&tu_ssv_cfg.rc_setting.sample_pr_5, 0, __string2u32 , "85"},
    { "sample_pr_force", (void *)&tu_ssv_cfg.rc_setting.force_sample_pr, 0, __string2u32 , "40"},
    { "greentx", (void *)&tu_ssv_cfg.greentx, 0, __string2u32 , "0x119"},
    { "gt_stepsize", (void *)&tu_ssv_cfg.gt_stepsize, 0, __string2u32 , "3"},
    { "gt_max_attenuation", (void *)&tu_ssv_cfg.gt_max_attenuation, 0, __string2u32 , "15"},
    { "autosgi", (void *)&tu_ssv_cfg.auto_sgi, 0, __string2u32 , "0x1"},
    { "directly_ack_low_threshold" ,(void *)&tu_ssv_cfg.directly_ack_low_threshold, 0, __string2u32 , "64"},
    { "directly_ack_high_threshold",(void *)&tu_ssv_cfg.directly_ack_high_threshold, 0, __string2u32 , "1024"},
    { "txrxboost_prio", (void *)&tu_ssv_cfg.txrxboost_prio, 0, __string2u32 , "20"},
    { "txrxboost_low_threshold",(void *)&tu_ssv_cfg.txrxboost_low_threshold, 0, __string2u32 , "64"},
    { "txrxboost_high_threshold",(void *)&tu_ssv_cfg.txrxboost_high_threshold, 0, __string2u32 , "256"},
    { "rx_threshold", (void *)&tu_ssv_cfg.rx_threshold, 0, __string2u32 , "64"},
    { "force_xtal_fo", (void *)&tu_ssv_cfg.force_xtal_fo, 0, __string2u32 , NULL},
    { "disable_dpd", (void *)&tu_ssv_cfg.disable_dpd, 0, __string2u32 , "0"},
    { "mic_err_notify", (void *)&tu_ssv_cfg.mic_err_notify, 0, __string2u32 , "0"},
    { "domain", (void *)&tu_ssv_cfg.domain, 0xff, __string2u32 , NULL},
    { NULL, NULL, 0, NULL, NULL},
};
EXPORT_SYMBOL(tu_cfg_cmds);
static int ssv_cmd_cfg(struct ssv_softc *sc, int argc, char *argv[])
{
    int s;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    if ((argc == 2) && (strcmp(argv[1], "reset") == 0)) {
        memset(&tu_ssv_cfg, 0, sizeof(tu_ssv_cfg));
        return 0;
    } else if ((argc == 2) && (strcmp(argv[1], "show") == 0)) {
        snprintf_res(cmd_data, ">> ssv6xxx config:\n");
        snprintf_res(cmd_data, "    hw_caps = 0x%08x\n", tu_ssv_cfg.hw_caps);
        snprintf_res(cmd_data, "    def_chan = %d\n", tu_ssv_cfg.def_chan);
        snprintf_res(cmd_data, "    wifi_tx_gain_level_gn = %d\n", tu_ssv_cfg.wifi_tx_gain_level_gn);
        snprintf_res(cmd_data, "    wifi_tx_gain_level_b = %d\n", tu_ssv_cfg.wifi_tx_gain_level_b);
        snprintf_res(cmd_data, "    sta-mac = %02x:%02x:%02x:%02x:%02x:%02x",
            tu_ssv_cfg.maddr[0][0], tu_ssv_cfg.maddr[0][1], tu_ssv_cfg.maddr[0][2],
            tu_ssv_cfg.maddr[0][3], tu_ssv_cfg.maddr[0][4], tu_ssv_cfg.maddr[0][5]);
        snprintf_res(cmd_data, "\n");
        return 0;
    }
    if (argc != 4)
        return -1;
    for(s = 0; tu_cfg_cmds[s].cfg_cmd != NULL; s++) {
        if (strcmp(tu_cfg_cmds[s].cfg_cmd, argv[1]) == 0) {
            tu_cfg_cmds[s].translate_func(argv[3],
                tu_cfg_cmds[s].var, tu_cfg_cmds[s].arg);
            snprintf_res(cmd_data, "");
            return 0;
        }
    }
    return -1;
}
#ifndef SSV_SUPPORT_HAL
void *ssv_dbg_phy_table = NULL;
EXPORT_SYMBOL(ssv_dbg_phy_table);
u32 ssv_dbg_phy_len = 0;
EXPORT_SYMBOL(ssv_dbg_phy_len);
void *ssv_dbg_rf_table = NULL;
EXPORT_SYMBOL(ssv_dbg_rf_table);
u32 ssv_dbg_rf_len = 0;
EXPORT_SYMBOL(ssv_dbg_rf_len);
#endif
void snprintf_res(struct ssv_cmd_data *cmd_data, const char *fmt, ... )
{
    char *buf_head;
    int buf_left;
    va_list args;
    char *ssv6xxx_result_buf = cmd_data->ssv6xxx_result_buf;
    ssv6xxx_result_buf = cmd_data->ssv6xxx_result_buf;
    if (cmd_data->rsbuf_len >= (cmd_data->rsbuf_size -1))
        return;
    buf_head = ssv6xxx_result_buf + cmd_data->rsbuf_len;
    buf_left = cmd_data->rsbuf_size - cmd_data->rsbuf_len;
    va_start(args, fmt);
    cmd_data->rsbuf_len += vsnprintf(buf_head, buf_left, fmt, args);
    va_end(args);
}
static void _dump_sta_info (struct ssv_softc *sc,
                            struct ssv_vif_info *vif_info,
                            struct ssv_sta_info *sta_info,
                            int sta_idx)
{
    struct ssv_sta_priv_data *priv_sta = (struct ssv_sta_priv_data *)sta_info->sta->drv_priv;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    if ((sta_info->s_flags & STA_FLAG_VALID) == 0) {
        snprintf_res(cmd_data,
                "        Station %d: %d is not valid\n",
                sta_idx, priv_sta->sta_idx);
    } else {
        snprintf_res(cmd_data,
                "        Station %d: %d\n"
                "             Address: %02X:%02X:%02X:%02X:%02X:%02X\n"
                "             WISD: %d\n"
                "             AID: %d\n",
                sta_idx, priv_sta->sta_idx,
                sta_info->sta->addr[0], sta_info->sta->addr[1], sta_info->sta->addr[2],
                sta_info->sta->addr[3], sta_info->sta->addr[4], sta_info->sta->addr[5],
                sta_info->hw_wsid, sta_info->aid);
    }
}
void ssv6xxx_dump_sta_info (struct ssv_softc *sc)
{
    int j, sta_idx = 0;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    snprintf_res(cmd_data, "  >>>> bcast queue len[%d]\n", sc->bcast_txq.cur_qsize);
    for (j = 0; j < SSV6200_MAX_VIF; j++) {
        struct ieee80211_vif *vif = sc->vif_info[j].vif;
        struct ssv_vif_priv_data *priv_vif;
        struct ssv_sta_priv_data *sta_priv_iter;
        if (vif == NULL){
            snprintf_res(cmd_data, "    VIF: %d is not used.\n", j);
            continue;
        }
         snprintf_res(cmd_data,
                 "    VIF: %d - [%02X:%02X:%02X:%02X:%02X:%02X] type[%d] p2p[%d] p2p_status[%d] channel[%d]\n", j,
                 vif->addr[0], vif->addr[1], vif->addr[2],
                 vif->addr[3], vif->addr[4], vif->addr[5], vif->type, vif->p2p, sc->p2p_status, sc->hw_chan);
        priv_vif = (struct ssv_vif_priv_data *)(vif->drv_priv);
         snprintf_res(cmd_data,
                 "           - sta asleep mask[%08X]\n", priv_vif->sta_asleep_mask);
        list_for_each_entry(sta_priv_iter, &priv_vif->sta_list, list){
            if ((sc->sta_info[sta_priv_iter->sta_idx].s_flags & STA_FLAG_VALID) == 0) {
                snprintf_res(cmd_data, "    STA: %d  is not valid.\n", sta_idx);
                continue;
            }
            _dump_sta_info(sc, &sc->vif_info[priv_vif->vif_idx],
                           &sc->sta_info[sta_priv_iter->sta_idx], sta_idx);
            sta_idx++;
        }
    }
}
static int ssv_cmd_sta(struct ssv_softc *sc, int argc, char *argv[])
{
    if ((argc >= 2) && (strcmp(argv[1], "show") == 0))
        ssv6xxx_dump_sta_info(sc);
    else
        snprintf_res(&sc->cmd_data, "sta show\n\n");
    return 0;
}
static void ssv_cmd_get_chip_id(struct ssv_softc *sc, char *chip_id)
{
#ifdef SSV_SUPPORT_HAL
    HAL_GET_CHIP_ID(sc->sh);
    strcpy(chip_id, sc->sh->chip_id);
#else
    u32 regval;
    if (SMAC_REG_READ(sc->sh, ADR_CHIP_ID_3, &regval));
    *((u32 *)&chip_id[0]) = __be32_to_cpu(regval);
    if (SMAC_REG_READ(sc->sh, ADR_CHIP_ID_2, &regval));
    *((u32 *)&chip_id[4]) = __be32_to_cpu(regval);
    if (SMAC_REG_READ(sc->sh, ADR_CHIP_ID_1, &regval));
    *((u32 *)&chip_id[8]) = __be32_to_cpu(regval);
    if (SMAC_REG_READ(sc->sh, ADR_CHIP_ID_0, &regval));
    *((u32 *)&chip_id[12]) = __be32_to_cpu(regval);
    chip_id[12+sizeof(u32)] = 0;
#endif
}
static bool ssv6xxx_dump_cfg(struct ssv_hw *sh)
{
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    snprintf_res(cmd_data, "\n>> Current Configuration:\n\n");
    snprintf_res(cmd_data, "  hw_mac:                   %pM\n", sh->cfg.maddr[0]);
    snprintf_res(cmd_data, "  hw_mac2:                  %pM\n", sh->cfg.maddr[1]);
    snprintf_res(cmd_data, "  def_chan:                 %d\n", sh->cfg.def_chan);
    snprintf_res(cmd_data, "  hw_caps:                  0x%x\n", sh->cfg.hw_caps);
    snprintf_res(cmd_data, "  use_wpa2_only:            %d\n", sh->cfg.use_wpa2_only);
    snprintf_res(cmd_data, "  wifi_tx_gain_level_gn:    %d\n", sh->cfg.wifi_tx_gain_level_gn);
    snprintf_res(cmd_data, "  wifi_tx_gain_level_b:     %d\n", sh->cfg.wifi_tx_gain_level_b);
    snprintf_res(cmd_data, "  xtal_clock:               %d\n", sh->cfg.crystal_type);
    snprintf_res(cmd_data, "  volt_regulator:           %d\n", sh->cfg.volt_regulator);
    snprintf_res(cmd_data, "  firmware_path:            %s\n", sh->cfg.firmware_path);
    snprintf_res(cmd_data, "  mac_address_path:         %s\n", sh->cfg.mac_address_path);
    snprintf_res(cmd_data, "  mac_output_path:          %s\n", sh->cfg.mac_output_path);
    snprintf_res(cmd_data, "  ignore_efuse_mac:         %d\n", sh->cfg.ignore_efuse_mac);
    snprintf_res(cmd_data, "  mac_address_mode:         %d\n", sh->cfg.mac_address_mode);
    snprintf_res(cmd_data, "  beacon_rssi_minimal:      %d\n", sh->cfg.beacon_rssi_minimal);
    snprintf_res(cmd_data, "  force_chip_identity:      0x%x\n", sh->cfg.force_chip_identity);
    snprintf_res(cmd_data, "  external_firmware_name:   %s\n", sh->cfg.external_firmware_name);
    snprintf_res(cmd_data, "  ignore_firmware_version:  %d\n", sh->cfg.ignore_firmware_version);
    snprintf_res(cmd_data, "  auto_rate_enable:         %d\n", sh->cfg.auto_rate_enable);
    snprintf_res(cmd_data, "  rc_rate_idx_set:          0x%x\n", sh->cfg.rc_rate_idx_set);
    snprintf_res(cmd_data, "  rc_retry_set:             0x%x\n", sh->cfg.rc_retry_set);
    snprintf_res(cmd_data, "  rc_mf:                    %d\n", sh->cfg.rc_mf);
    snprintf_res(cmd_data, "  rc_long_short:            %d\n", sh->cfg.rc_long_short);
    snprintf_res(cmd_data, "  rc_ht40:                  %d\n", sh->cfg.rc_ht40);
    snprintf_res(cmd_data, "  rc_phy_mode:              %d\n", sh->cfg.rc_phy_mode);
    snprintf_res(cmd_data, "  cci:                      %d\n", sh->cfg.cci);
#ifdef SSV_SUPPORT_HAL
    snprintf_res(cmd_data, "  hwqlimit(BK queue):        %d\n", sh->cfg.bk_txq_size);
    snprintf_res(cmd_data, "  hwqlimit(BE queue):        %d\n", sh->cfg.be_txq_size);
    snprintf_res(cmd_data, "  hwqlimit(VI queue):        %d\n", sh->cfg.vi_txq_size);
    snprintf_res(cmd_data, "  hwqlimit(VO queue):        %d\n", sh->cfg.vo_txq_size);
    snprintf_res(cmd_data, "  hwqlimit(MNG queue):       %d\n", sh->cfg.manage_txq_size);
#endif
    snprintf_res(cmd_data, "\n\n");
    return 0;
}
static int ssv_cmd_dump(struct ssv_softc *sc, int argc, char *argv[])
{
#ifndef SSV_SUPPORT_HAL
    u32 regval;
    int s;
#endif
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    if (argc != 2) {
        snprintf_res(cmd_data, "dump [wsid|decision|phy-reg|rf-reg|cfg]\n");
        return 0;
    }
    if (strcmp(argv[1], "wsid") == 0) {
       return SSV_DUMP_WSID(sc->sh);
    }
    if (strcmp(argv[1], "decision") == 0 ) {
        return SSV_DUMP_DECISION(sc->sh);
    }
    if (strcmp(argv[1], "phy-info") == 0) {
        return 0;
    }
    if (strcmp(argv[1], "phy-reg") == 0) {
#ifdef SSV_SUPPORT_HAL
        return HAL_DUMP_PHY_REG(sc->sh);
#else
        int length;
        struct ssv6xxx_dev_table *raw;
        raw = (struct ssv6xxx_dev_table *) ssv_dbg_phy_table;
        length = ssv_dbg_phy_len;
        snprintf_res(cmd_data, ">> PHY Register Table:\n");
        for(s = 0; s < length; s++, raw++) {
            if(SMAC_REG_READ(sc->sh, raw->address, &regval));
            snprintf_res(cmd_data, "   ADDR[0x%08x] = 0x%08x\n",
            raw->address, regval);
        }
        snprintf_res(cmd_data, "\n\n");
        return 0;
#endif
    }
    if (strcmp(argv[1], "rf-reg") == 0) {
#ifdef SSV_SUPPORT_HAL
        return HAL_DUMP_RF_REG(sc->sh);
#else
        int length;
        struct ssv6xxx_dev_table *raw;
        raw = (struct ssv6xxx_dev_table *)ssv_dbg_rf_table;
        length = ssv_dbg_rf_len;
        snprintf_res(cmd_data, ">> RF Register Table:\n");
        for(s = 0; s < length ; s++, raw++) {
            if (SMAC_REG_READ(sc->sh, raw->address, &regval));
            snprintf_res(cmd_data, "   ADDR[0x%08x] = 0x%08x\n",
            raw->address, regval);
        }
        snprintf_res(cmd_data, "\n\n");
        return 0;
#endif
    }
    if (strcmp(argv[1], "cfg") == 0) {
        return ssv6xxx_dump_cfg(sc->sh);
    }
    snprintf_res(cmd_data, "dump [wsid|decision|phy-reg|rf-reg|cfg]\n");
    return 0;
}
static int ssv_cmd_irq(struct ssv_softc *sc, int argc, char *argv[])
{
    char *endp;
    u32 irq_sts;
    struct ssv6xxx_hci_info *hci = &sc->sh->hci;
    struct ssv6xxx_hwif_ops *ifops = hci->if_ops;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    if ((argc >= 3) && (strcmp(argv[1], "set") == 0)) {
        if ((strcmp(argv[2], "mask") == 0) && (argc == 4)) {
            irq_sts = simple_strtoul(argv[3], &endp, 16);
      if (!sc->sh->hci.if_ops->irq_setmask) {
             snprintf_res(cmd_data, "The interface doesn't provide irq_setmask operation.\n");
             return 0;
      }
            ifops->irq_setmask(hci->dev, irq_sts);
            snprintf_res(cmd_data, "set sdio irq mask to 0x%08x\n", irq_sts);
            return 0;
        }
        if (strcmp(argv[2], "enable") == 0) {
         if (!ifops->irq_enable) {
             snprintf_res(cmd_data, "The interface doesn't provide irq_enable operation.\n");
             return 0;
      }
            ifops->irq_enable(hci->dev);
            snprintf_res(cmd_data, "enable sdio irq.\n");
            return 0;
        }
        if (strcmp(argv[2], "disable") == 0) {
      if (!ifops->irq_disable) {
             snprintf_res(cmd_data, "The interface doesn't provide irq_disable operation.\n");
             return 0;
      }
            ifops->irq_disable(hci->dev, false);
            snprintf_res(cmd_data, "disable sdio irq.\n");
            return 0;
        }
        return -1;
    } else if ( (argc == 3) && (strcmp(argv[1], "get") == 0)) {
        if (strcmp(argv[2], "mask") == 0) {
      if (!ifops->irq_getmask) {
                snprintf_res(cmd_data, "The interface doesn't provide irq_getmask operation.\n");
             return 0;
      }
            ifops->irq_getmask(hci->dev, &irq_sts);
            snprintf_res(cmd_data, "sdio irq mask: 0x%08x, int_mask=0x%08x\n", irq_sts,
                         hci->hci_ctrl->int_mask);
            return 0;
        }
        if (strcmp(argv[2], "status") == 0) {
      if (!ifops->irq_getstatus) {
             snprintf_res(cmd_data, "The interface doesn't provide irq_getstatus operation.\n");
             return 0;
      }
            ifops->irq_getstatus(hci->dev, &irq_sts);
            snprintf_res(cmd_data, "sdio irq status: 0x%08x\n", irq_sts);
            return 0;
        }
        return -1;
    } else {
        snprintf_res(cmd_data, "irq [set|get] [mask|enable|disable|status]\n");
    }
    return 0;
}
static int ssv_cmd_mac(struct ssv_softc *sc, int argc, char *argv[])
{
    char *endp;
    int i;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
#if 0
    if ((argc == 3) && (!strcmp(argv[1], "wsid")) && (!strcmp(argv[2], "show"))) {
        u32 s;
        }
        return 0;
    } else
#endif
    if ((argc == 3) && (!strcmp(argv[1], "rx"))){
        if (!strcmp(argv[2], "enable")){
            sc->dbg_rx_frame = 1;
        } else {
            sc->dbg_rx_frame = 0;
        }
        snprintf_res(cmd_data, "  dbg_rx_frame %d\n", sc->dbg_rx_frame);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "tx"))){
        if(!strcmp(argv[2], "enable")){
            sc->dbg_tx_frame = 1;
        } else {
            sc->dbg_tx_frame = 0;
        }
        snprintf_res(cmd_data, "  dbg_tx_frame %d\n", sc->dbg_tx_frame);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "rxq")) && (!strcmp(argv[2], "show"))) {
        snprintf_res(cmd_data, ">> MAC RXQ: (%s)\n    cur_qsize=%d\n",
                     ((sc->sc_flags & SC_OP_OFFCHAN)? "off channel": "on channel"),
                     sc->rx.rxq_count);
        return 0;
    }
#if 0
    if (argc==3 && !strcmp(argv[1], "tx") && !strcmp(argv[2], "status")) {
        snprintf_res(cmd_data, ">> MAC TX Status:\n");
        snprintf_res(cmd_data, "    txq flow control: 0x%x\n", sc->tx.flow_ctrl_status);
        snprintf_res(cmd_data, "    rxq cur_qsize: %d\n", sc->rx.rxq_count);
    }
#endif
    else if ((argc == 4) && (!strcmp(argv[1], "set")) && (!strcmp(argv[2], "rate"))) {
        if (strcmp(argv[3], "auto") == 0) {
            sc->sc_flags &= ~SC_OP_FIXED_RATE;
            return 0;
        }
        i = simple_strtoul(argv[3], &endp, 10);
        if ( i < 0 || i > 38) {
            snprintf_res(cmd_data, " Invalid rat index !!\n");
            return -1;
        }
        sc->max_rate_idx = i;
        sc->sc_flags |= SC_OP_FIXED_RATE;
        snprintf_res(cmd_data, " Set rate to index %d\n", i);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "get")) && (!strcmp(argv[2], "rate"))) {
        if (sc->sc_flags & SC_OP_FIXED_RATE)
            snprintf_res(cmd_data, " Current Rate Index: %d\n", sc->max_rate_idx);
        else
            snprintf_res(cmd_data, "  Current Rate Index: auto\n");
        return 0;
    } else if ((argc == 2) && (!strcmp(argv[1], "setting"))){
        snprintf_res(cmd_data, "tx_cfg threshold:\n");
        snprintf_res(cmd_data, "\t tx_id_threshold %d tx_cfg tx_lowthreshold_id_trigger %d tx_page_threshold %d\n",
            sc->sh->tx_info.tx_id_threshold, sc->sh->tx_info.tx_lowthreshold_id_trigger,
            sc->sh->tx_info.tx_page_threshold);
        snprintf_res(cmd_data, "rx_cfg threshold:\n");
        snprintf_res(cmd_data, "\t rx_id_threshold %d  rx_page_threshold %d\n",
            sc->sh->rx_info.rx_id_threshold, sc->sh->rx_info.rx_page_threshold);
        snprintf_res(cmd_data, "tx page available %d\n" , sc->sh->tx_page_available);
    } else {
        snprintf_res(cmd_data, "mac [rxq] [show]\n");
        snprintf_res(cmd_data, "mac [set|get] [rate] [auto|idx]\n");
        snprintf_res(cmd_data, "mac [rx|tx] [eable|disable]\n");
        snprintf_res(cmd_data, "mac [setting]\n");
    }
    return 0;
}
#ifdef CONFIG_IRQ_DEBUG_COUNT
void print_irq_count(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    struct ssv_cmd_data *cmd_data = &hci_ctrl->shi->sc->cmd_data;
 snprintf_res(cmd_data, "irq debug (%s)\n", hci_ctrl->irq_enable?"enable":"disable");
    snprintf_res(cmd_data, "total irq (%d)\n", hci_ctrl->irq_count);
    snprintf_res(cmd_data, "invalid irq (%d)\n", hci_ctrl->invalid_irq_count);
 snprintf_res(cmd_data, "rx irq (%d)\n", hci_ctrl->rx_irq_count);
 snprintf_res(cmd_data, "tx irq (%d)\n", hci_ctrl->tx_irq_count);
 snprintf_res(cmd_data, "real tx count irq (%d)\n", hci_ctrl->real_tx_irq_count);
 snprintf_res(cmd_data, "tx  packet count (%d)\n", hci_ctrl->irq_tx_pkt_count);
 snprintf_res(cmd_data, "rx packet (%d)\n", hci_ctrl->irq_rx_pkt_count);
}
#endif
void print_isr_info(struct ssv_cmd_data *cmd_data, struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    snprintf_res(cmd_data, ">>>> HCI Calculate ISR TIME(%s) unit:us\n",
                ((hci_ctrl->isr_summary_eable) ? "enable": "disable"));
    snprintf_res(cmd_data, "isr_routine_time(%d)\n",
                 jiffies_to_usecs(hci_ctrl->isr_routine_time));
    snprintf_res(cmd_data, "isr_tx_time(%d)\n",
                 jiffies_to_usecs(hci_ctrl->isr_tx_time));
    snprintf_res(cmd_data, "isr_rx_time(%d)\n",
                 jiffies_to_usecs(hci_ctrl->isr_rx_time));
    snprintf_res(cmd_data, "isr_idle_time(%d)\n",
                 jiffies_to_usecs(hci_ctrl->isr_idle_time));
    snprintf_res(cmd_data, "isr_rx_idle_time(%d)\n",
                 jiffies_to_usecs(hci_ctrl->isr_rx_idle_time));
    snprintf_res(cmd_data, "isr_miss_cnt(%d)\n",
                 hci_ctrl->isr_miss_cnt);
    snprintf_res(cmd_data, "prev_isr_jiffes(%lu)\n",
                 hci_ctrl->prev_isr_jiffes);
    snprintf_res(cmd_data, "prev_rx_isr_jiffes(%lu)\n",
                 hci_ctrl->prev_rx_isr_jiffes);
}
static int ssv_cmd_hci(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv6xxx_hci_ctrl *hci_ctrl = sc->sh->hci.hci_ctrl;
    struct ssv_hw_txq *txq;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    int s, ac = 0;
    if ((argc == 3) && (!strcmp(argv[1], "txq")) && (!strcmp(argv[2], "show"))) {
        for(s = 0; s < WMM_NUM_AC; s++) {
            ac = sc->tx.ac_txqid[s];
            txq = &hci_ctrl->hw_txq[s];
            snprintf_res(cmd_data, ">> txq[%d]", txq->txq_no);
            snprintf_res(cmd_data, "(%s): ",
                         ( (sc->sc_flags & SC_OP_OFFCHAN)
                          ? "off channel"
                          : "on channel"));
            snprintf_res(cmd_data, "cur_qsize=%d\n", skb_queue_len(&txq->qhead));
            snprintf_res(cmd_data, "            max_qsize=%d, pause=%d, resume_thres=%d",
                         txq->max_qsize, txq->paused, txq->resum_thres);
            snprintf_res(cmd_data, " flow_control[%d]\n",
                         (sc->tx.flow_ctrl_status & (1<<ac)));
            snprintf_res(cmd_data, "            Total %d frame sent\n",
                         txq->tx_pkt);
        }
        snprintf_res(cmd_data,
                     ">> HCI Debug Counters:\n    read_rs0_info_fail=%d, read_rs1_info_fail=%d\n",
                     hci_ctrl->read_rs0_info_fail, hci_ctrl->read_rs1_info_fail);
        snprintf_res(cmd_data,
                     "    rx_work_running=%d, isr_running=%d, xmit_running=%d\n",
                     hci_ctrl->rx_work_running, hci_ctrl->isr_running,
                     hci_ctrl->xmit_running);
        snprintf_res(cmd_data, "    flow_ctrl_status=%08x\n", sc->tx.flow_ctrl_status);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "rxq")) && (!strcmp(argv[2], "show"))){
        snprintf_res(cmd_data, ">> HCI RX Queue (%s): cur_qsize=%d\n",
                     ((sc->sc_flags & SC_OP_OFFCHAN)? "off channel": "on channel"),
                     hci_ctrl->rx_pkt);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "isr_time")) && (!strcmp(argv[2], "start"))) {
        hci_ctrl->isr_summary_eable = 1;
        hci_ctrl->isr_routine_time = 0;
        hci_ctrl->isr_tx_time = 0;
        hci_ctrl->isr_rx_time = 0;
        hci_ctrl->isr_idle_time = 0;
        hci_ctrl->isr_rx_idle_time = 0;
        hci_ctrl->isr_miss_cnt = 0;
        hci_ctrl->prev_isr_jiffes = 0;
        hci_ctrl->prev_rx_isr_jiffes = 0;
        print_isr_info(cmd_data, hci_ctrl);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "isr_time")) && (!strcmp(argv[2], "stop"))) {
        hci_ctrl->isr_summary_eable = 0;
        print_isr_info(cmd_data, hci_ctrl);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "isr_time")) && (!strcmp(argv[2], "show"))) {
        print_isr_info(cmd_data, hci_ctrl);
        return 0;
    }
#ifdef CONFIG_IRQ_DEBUG_COUNT
    else if ((argc == 3) && (!strcmp(argv[1], "isr_debug")) && (!strcmp(argv[2], "reset"))) {
        hci_ctrl->irq_enable= 0;
        hci_ctrl->irq_count = 0;
        hci_ctrl->invalid_irq_count = 0;
        hci_ctrl->tx_irq_count = 0;
        hci_ctrl->real_tx_irq_count = 0;
        hci_ctrl->rx_irq_count = 0;
        hci_ctrl->isr_rx_idle_time = 0;
        hci_ctrl->irq_rx_pkt_count = 0;
        hci_ctrl->irq_tx_pkt_count = 0;
        snprintf_res(cmd_data, "irq debug reset count\n");
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "isr_debug")) && (!strcmp(argv[2], "show"))) {
  print_irq_count(hci_ctrl);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "isr_debug")) && (!strcmp(argv[2], "stop"))) {
        hci_ctrl->irq_enable= 0;
  snprintf_res(cmd_data, "irq debug stop\n");
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "isr_debug")) && (!strcmp(argv[2], "start"))) {
        hci_ctrl->irq_enable= 1;
  snprintf_res(cmd_data, "irq debug start\n");
        return 0;
    }
#endif
    else
    {
        snprintf_res(cmd_data,
                     "hci [txq|rxq] [show]\nhci [isr_time] [start|stop|show]\n\n");
        return 0;
    }
    return -1;
}
static int ssv_cmd_hwq(struct ssv_softc *sc, int argc, char *argv[])
{
     struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    u32 value, value1, value2;
 u32 tx_len = 0, rx_len = 0, ava_status = 0;
    SSV_READ_FFOUT_CNT(sc->sh, &value, &value1, &value2);
    snprintf_res(cmd_data, "\n[TAG]  MCU - HCI - SEC -  RX - MIC - TX0 - TX1 - TX2 - TX3 - TX4 - SEC - MIC - TSH\n");
    snprintf_res(cmd_data, "OUTPUT %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d\n",
   SSV_GET_FFOUT_CNT(sc->sh, value, M_ENG_CPU),
   SSV_GET_FFOUT_CNT(sc->sh, value, M_ENG_HWHCI),
   SSV_GET_FFOUT_CNT(sc->sh, value, M_ENG_ENCRYPT),
   SSV_GET_FFOUT_CNT(sc->sh, value, M_ENG_MACRX),
   SSV_GET_FFOUT_CNT(sc->sh, value, M_ENG_MIC),
   SSV_GET_FFOUT_CNT(sc->sh, value1, M_ENG_TX_EDCA0),
   SSV_GET_FFOUT_CNT(sc->sh, value1, M_ENG_TX_EDCA1),
   SSV_GET_FFOUT_CNT(sc->sh, value1, M_ENG_TX_EDCA2),
   SSV_GET_FFOUT_CNT(sc->sh, value1, M_ENG_TX_EDCA3),
   SSV_GET_FFOUT_CNT(sc->sh, value1, M_ENG_TX_MNG),
   SSV_GET_FFOUT_CNT(sc->sh, value1, M_ENG_ENCRYPT_SEC),
   SSV_GET_FFOUT_CNT(sc->sh, value2, M_ENG_MIC_SEC),
   SSV_GET_FFOUT_CNT(sc->sh, value2, M_ENG_TRASH_CAN));
    SSV_READ_IN_FFCNT(sc->sh, &value, &value1);
    snprintf_res(cmd_data, "INPUT  %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d - %3d\n",
   SSV_GET_IN_FFCNT(sc->sh, value, M_ENG_CPU),
   SSV_GET_IN_FFCNT(sc->sh, value, M_ENG_HWHCI),
   SSV_GET_IN_FFCNT(sc->sh, value, M_ENG_ENCRYPT),
   SSV_GET_IN_FFCNT(sc->sh, value1, M_ENG_MACRX),
   SSV_GET_IN_FFCNT(sc->sh, value, M_ENG_MIC),
   SSV_GET_IN_FFCNT(sc->sh, value, M_ENG_TX_EDCA0),
   SSV_GET_IN_FFCNT(sc->sh, value, M_ENG_TX_EDCA1),
   SSV_GET_IN_FFCNT(sc->sh, value, M_ENG_TX_EDCA2),
   SSV_GET_IN_FFCNT(sc->sh, value, M_ENG_TX_EDCA3),
   SSV_GET_IN_FFCNT(sc->sh, value1, M_ENG_TX_MNG),
   SSV_GET_IN_FFCNT(sc->sh, value1, M_ENG_ENCRYPT_SEC),
   SSV_GET_IN_FFCNT(sc->sh, value1, M_ENG_MIC_SEC),
   SSV_GET_IN_FFCNT(sc->sh, value1, M_ENG_TRASH_CAN));
    SSV_READ_ID_LEN_THRESHOLD(sc->sh, &tx_len, &rx_len);
    SSV_READ_TAG_STATUS(sc->sh, &ava_status);
    snprintf_res(cmd_data, "TX[%d]RX[%d]AVA[%d]\n", tx_len, rx_len, ava_status);
    return 0;
}
#ifdef CONFIG_P2P_NOA
#if 0
struct ssv6xxx_p2p_noa_param {
    u32 duration;
    u32 interval;
    u32 start_time;
    u32 enable:8;
    u32 count:8;
    u8 addr[6];
};
#endif
static struct ssv6xxx_p2p_noa_param cmd_noa_param = {
    50,
    100,
    0x12345678,
    1,
    255,
    {0x4c, 0xe6, 0x76, 0xa2, 0x4e, 0x7c}
};
void noa_dump(struct ssv_cmd_data *cmd_data)
{
    snprintf_res(cmd_data, "NOA Parameter:\nEnable=%d\nInterval=%d\nDuration=%d\nStart_time=0x%08x\nCount=%d\nAddr=[%02x:%02x:%02x:%02x:%02x:%02x]\n",
                                cmd_noa_param.enable,
                                cmd_noa_param.interval,
                                cmd_noa_param.duration,
                                cmd_noa_param.start_time,
                                cmd_noa_param.count,
                                cmd_noa_param.addr[0],
                                cmd_noa_param.addr[1],
                                cmd_noa_param.addr[2],
                                cmd_noa_param.addr[3],
                                cmd_noa_param.addr[4],
                                cmd_noa_param.addr[5]);
}
void ssv6xxx_send_noa_cmd(struct ssv_softc *sc, struct ssv6xxx_p2p_noa_param *p2p_noa_param)
{
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    int retry_cnt = 5;
    skb = ssvdevice_skb_alloc(HOST_CMD_HDR_LEN + sizeof(struct ssv6xxx_p2p_noa_param));
    skb->data_len = HOST_CMD_HDR_LEN + sizeof(struct ssv6xxx_p2p_noa_param);
    skb->len = skb->data_len;
    host_cmd = (struct cfg_host_cmd *)skb->data;
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_SET_NOA;
    host_cmd->len = skb->data_len;
    memcpy(host_cmd->dat32, p2p_noa_param, sizeof(struct ssv6xxx_p2p_noa_param));
    while((HCI_SEND_CMD(sc->sh, skb)!=0)&&(retry_cnt)){
        printk(KERN_INFO "NOA cmd retry=%d!!\n",retry_cnt);
        retry_cnt--;
    }
    ssvdevice_skb_free(skb);
}
static int ssv_cmd_noa(struct ssv_softc *sc, int argc, char *argv[])
{
    char *endp = NULL;
    if ( (argc == 2) && (!strcmp(argv[1], "show"))) {
     ;
    }else if ((argc == 3) && (!strcmp(argv[1], "duration"))){
        cmd_noa_param.duration= simple_strtoul(argv[2], &endp, 0);
    }else if ((argc == 3) && (!strcmp(argv[1], "interval"))) {
        cmd_noa_param.interval= simple_strtoul(argv[2], &endp, 0);
    }else if ((argc == 3) && (!strcmp(argv[1], "start"))) {
        cmd_noa_param.start_time= simple_strtoul(argv[2], &endp, 0);
    }else if ((argc == 3) && (!strcmp(argv[1], "enable"))) {
        cmd_noa_param.enable= simple_strtoul(argv[2], &endp, 0);
    }else if ((argc == 3) && (!strcmp(argv[1], "count"))) {
         cmd_noa_param.count= simple_strtoul(argv[2], &endp, 0);
    }else if ((argc == 8) && (!strcmp(argv[1], "addr"))) {
         cmd_noa_param.addr[0]= simple_strtoul(argv[2], &endp, 16);
         cmd_noa_param.addr[1]= simple_strtoul(argv[3], &endp, 16);
         cmd_noa_param.addr[2]= simple_strtoul(argv[4], &endp, 16);
         cmd_noa_param.addr[3]= simple_strtoul(argv[5], &endp, 16);
         cmd_noa_param.addr[4]= simple_strtoul(argv[6], &endp, 16);
         cmd_noa_param.addr[5]= simple_strtoul(argv[7], &endp, 16);
    }else if ((argc == 2) && (!strcmp(argv[1], "send"))) {
        ssv6xxx_send_noa_cmd(sc, &cmd_noa_param);
    }else{
        snprintf_res(&sc->cmd_data, "## wrong command\n");
        return 0;
    }
    noa_dump(&sc->cmd_data);
    return 0;
}
#endif
static int ssv_cmd_mib(struct ssv_softc *sc, int argc, char *argv[])
{
    SSV_CMD_MIB(sc, argc, argv);
    return 0;
}
static int ssv_cmd_power_saving(struct ssv_softc *sc, int argc, char *argv[])
{
    SSV_CMD_POWER_SAVING(sc, argc, argv);
    return 0;
}
static int ssv_cmd_sdio(struct ssv_softc *sc, int argc, char *argv[])
{
    u32 addr, value;
    char *endp;
    int ret=0;
    struct ssv6xxx_hci_info *hci = &sc->sh->hci;
    struct ssv6xxx_hwif_ops *ifops = hci->if_ops;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    if ((argc == 4) && (!strcmp(argv[1], "reg")) && (!strcmp(argv[2], "r"))) {
        addr = simple_strtoul(argv[3], &endp, 16);
   if (!ifops->cmd52_read){
      snprintf_res(cmd_data, "The interface doesn't provide cmd52 read\n");
   return 0;
  }
  ret = ifops->cmd52_read(hci->dev, addr, &value);
        if (ret >= 0) {
            snprintf_res(cmd_data, "  ==> %x\n", value);
            return 0;
        }
    } else if ((argc ==5) && (!strcmp(argv[1], "reg")) && (!strcmp(argv[2], "w"))){
        addr = simple_strtoul(argv[3], &endp, 16);
        value = simple_strtoul(argv[4], &endp, 16);
        if (!ifops->cmd52_write){
      snprintf_res(cmd_data, "The interface doesn't provide cmd52 write\n");
   return 0;
  }
        ret = ifops->cmd52_write(hci->dev, addr, value);
        if (ret >= 0) {
            snprintf_res(cmd_data, "  ==> write done.\n");
            return 0;
        }
    }
    snprintf_res(cmd_data, "sdio cmd52 fail: %d\n", ret);
    return 0;
}
#if (defined (SSV_SUPPORT_HAL) || defined ( CONFIG_SSV_CABRIO_E))
static struct ssv6xxx_iqk_cfg cmd_iqk_cfg = {
    SSV6XXX_IQK_CFG_XTAL_26M,
    SSV6XXX_IQK_CFG_PA_DEF,
    0,
    0,
    26,
    3,
    0x75,
    0x75,
    0x80,
    0x80,
    SSV6XXX_IQK_CMD_INIT_CALI,
    { SSV6XXX_IQK_TEMPERATURE
    + SSV6XXX_IQK_RXDC
    + SSV6XXX_IQK_RXRC
    + SSV6XXX_IQK_TXDC
    + SSV6XXX_IQK_TXIQ
    + SSV6XXX_IQK_RXIQ
    },
};
static int ssv_cmd_iqk (struct ssv_softc *sc, int argc, char *argv[]) {
    char *endp;
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    u32 rxcnt_total, rxcnt_error;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
#ifdef SSV_SUPPORT_HAL
    if (HAL_SUPPORT_IQK_CMD(sc->sh) == false){
        snprintf_res(cmd_data, "iqk command is not supported for this chip\n");
        return 0;
    }
#else
    char chip_id[24]="";
    ssv_cmd_get_chip_id(sc, chip_id);
    if (!(strstr(chip_id, SSV6051_CHIP))){
        snprintf_res(cmd_data, "iqk command is not supported for this chip\n");
        return 0;
    }
#endif
    snprintf_res(cmd_data, "# got iqk command\n");
    if ((argc == 3) && (strcmp(argv[1], "cfg-pa") == 0)) {
        cmd_iqk_cfg.cfg_pa = simple_strtoul(argv[2], &endp, 0);
        snprintf_res(cmd_data, "## set cfg_pa as %d\n", cmd_iqk_cfg.cfg_pa);
        return 0;
    } else if ((argc == 3) && (strcmp(argv[1], "cfg-tssi-trgt") == 0)) {
        cmd_iqk_cfg.cfg_tssi_trgt = simple_strtoul(argv[2], &endp, 0);
        snprintf_res(cmd_data, "## set cfg_tssi_trgt as %d\n", cmd_iqk_cfg.cfg_tssi_trgt);
        return 0;
    } else if ((argc == 3) && (strcmp(argv[1], "init-cali") == 0)) {
        cmd_iqk_cfg.cmd_sel = SSV6XXX_IQK_CMD_INIT_CALI;
        cmd_iqk_cfg.fx_sel = simple_strtoul(argv[2], &endp, 0);
        snprintf_res(cmd_data, "## do init-cali\n");
    } else if ((argc == 3) && (strcmp(argv[1], "rtbl-load") == 0)) {
        cmd_iqk_cfg.cmd_sel = SSV6XXX_IQK_CMD_RTBL_LOAD;
        cmd_iqk_cfg.fx_sel = simple_strtoul(argv[2], &endp, 0);
        snprintf_res(cmd_data, "## do rtbl-load\n");
    } else if ((argc == 3) && (strcmp(argv[1], "rtbl-load-def") == 0)) {
        cmd_iqk_cfg.cmd_sel = SSV6XXX_IQK_CMD_RTBL_LOAD_DEF;
        cmd_iqk_cfg.fx_sel = simple_strtoul(argv[2], &endp, 0);
        snprintf_res(cmd_data, "## do rtbl-load\n");
    } else if ((argc == 3) && (strcmp(argv[1], "rtbl-reset") == 0)) {
        cmd_iqk_cfg.cmd_sel = SSV6XXX_IQK_CMD_RTBL_RESET;
        cmd_iqk_cfg.fx_sel = simple_strtoul(argv[2], &endp, 0);
        snprintf_res(cmd_data, "## do rtbl-reset\n");
    } else if ((argc == 3) && (strcmp(argv[1], "rtbl-set") == 0)) {
        cmd_iqk_cfg.cmd_sel = SSV6XXX_IQK_CMD_RTBL_SET;
        cmd_iqk_cfg.fx_sel = simple_strtoul(argv[2], &endp, 0);
        snprintf_res(cmd_data, "## do rtbl-set\n");
    } else if ((argc == 3) && (strcmp(argv[1], "rtbl-export") == 0)) {
        cmd_iqk_cfg.cmd_sel = SSV6XXX_IQK_CMD_RTBL_EXPORT;
        cmd_iqk_cfg.fx_sel = simple_strtoul(argv[2], &endp, 0);
        snprintf_res(cmd_data, "## do rtbl-export\n");
    } else if ((argc == 3) && (strcmp(argv[1], "tk-evm") == 0)) {
        cmd_iqk_cfg.cmd_sel = SSV6XXX_IQK_CMD_TK_EVM;
        cmd_iqk_cfg.argv = simple_strtoul(argv[2], &endp, 0);
        snprintf_res(cmd_data, "## do tk-evm\n");
    } else if ((argc == 3) && (strcmp(argv[1], "tk-tone") == 0)) {
        cmd_iqk_cfg.cmd_sel = SSV6XXX_IQK_CMD_TK_TONE;
        cmd_iqk_cfg.argv = simple_strtoul(argv[2], &endp, 0);
        snprintf_res(cmd_data, "## do tk-tone\n");
    } else if ((argc == 3) && (strcmp(argv[1], "channel") == 0)) {
        cmd_iqk_cfg.cmd_sel = SSV6XXX_IQK_CMD_TK_CHCH;
        cmd_iqk_cfg.argv = simple_strtoul(argv[2], &endp, 0);
        snprintf_res(cmd_data, "## do change channel\n");
    } else if ((argc == 2) && (strcmp(argv[1], "tk-rxcnt-report") == 0)) {
        if(SMAC_REG_READ(sc->sh, 0xCE0043E8, &rxcnt_error));
        if(SMAC_REG_READ(sc->sh, 0xCE0043EC, &rxcnt_total));
        snprintf_res(cmd_data, "## GN Rx error rate = (%06d/%06d)\n", rxcnt_error, rxcnt_total);
        if(SMAC_REG_READ(sc->sh, 0xCE0023E8, &rxcnt_error));
        if(SMAC_REG_READ(sc->sh, 0xCE0023EC, &rxcnt_total));
        snprintf_res(cmd_data, "## B Rx error rate = (%06d/%06d)\n", rxcnt_error, rxcnt_total);
        return 0;
    } else {
        snprintf_res(cmd_data, "## invalid iqk command\n");
        snprintf_res(cmd_data, "## cmd: cfg-pa/cfg-tssi-trgt\n");
        snprintf_res(cmd_data, "## cmd: init-cali/rtbl-load/rtbl-load-def/rtbl-reset/rtbl-set/rtbl-export/tk-evm/tk-tone/tk-channel\n");
        snprintf_res(cmd_data, "## fx_sel: 0x0008: RXDC\n");
        snprintf_res(cmd_data, "           0x0010: RXRC\n");
        snprintf_res(cmd_data, "           0x0020: TXDC\n");
        snprintf_res(cmd_data, "           0x0040: TXIQ\n");
        snprintf_res(cmd_data, "           0x0080: RXIQ\n");
        snprintf_res(cmd_data, "           0x0100: TSSI\n");
        snprintf_res(cmd_data, "           0x0200: PAPD\n");
        return 0;
    }
 {
        int phy_setting_size, rf_setting_size;
        ssv_cabrio_reg *phy_tbl, *rf_tbl ;
#ifdef SSV_SUPPORT_HAL
        phy_setting_size = HAL_GET_PHY_TABLE_SIZE(sc->sh);
        rf_setting_size = HAL_GET_RF_TABLE_SIZE(sc->sh);
        HAL_LOAD_PHY_TABLE(sc->sh, &phy_tbl);
        HAL_LOAD_RF_TABLE(sc->sh, &rf_tbl);
#else
        phy_setting_size = PHY_SETTING_SIZE;
        rf_setting_size = RF_SETTING_SIZE;
        phy_tbl = phy_setting;
        rf_tbl = asic_rf_setting;
#endif
        skb = ssvdevice_skb_alloc(HOST_CMD_HDR_LEN + IQK_CFG_LEN + phy_setting_size + rf_setting_size);
        if(skb == NULL) {
            printk("ssv command ssvdevice_skb_alloc fail!!!\n");
            return 0;
        }
        if((phy_setting_size > MAX_PHY_SETTING_TABLE_SIZE) ||
             (rf_setting_size > MAX_RF_SETTING_TABLE_SIZE)) {
            printk("Please check RF or PHY table size!!!\n");
            BUG_ON(1);
            return 0;
        }
        skb->data_len = HOST_CMD_HDR_LEN + IQK_CFG_LEN + phy_setting_size + rf_setting_size;
        skb->len = skb->data_len;
        host_cmd = (struct cfg_host_cmd *)skb->data;
        host_cmd->c_type = HOST_CMD;
        host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_INIT_CALI;
        host_cmd->len = skb->data_len;
        cmd_iqk_cfg.phy_tbl_size = phy_setting_size;
        cmd_iqk_cfg.rf_tbl_size = rf_setting_size;
        memcpy(host_cmd->dat32, &cmd_iqk_cfg, IQK_CFG_LEN);
        memcpy(host_cmd->dat8+IQK_CFG_LEN, phy_tbl, phy_setting_size);
        memcpy(host_cmd->dat8+IQK_CFG_LEN+phy_setting_size, rf_tbl, rf_setting_size);
        if (sc->sh->hci.hci_ops->hci_send_cmd(sc->sh->hci.hci_ctrl, skb) == 0) {
            snprintf_res(cmd_data, "## hci send cmd success\n");
        } else {
            snprintf_res(cmd_data, "## hci send cmd fail\n");
        }
        ssvdevice_skb_free(skb);
        return 0;
    }
}
#endif
#define CLI_VERSION "0.08"
static int ssv_cmd_version (struct ssv_softc *sc, int argc, char *argv[]) {
    u32 regval;
    char chip_id[24] = "";
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    snprintf_res(cmd_data, "CLI version: %s \n", CLI_VERSION);
    snprintf_res(cmd_data, "CHIP TAG: %llx \n", sc->sh->chip_tag);
    ssv_cmd_get_chip_id(sc, chip_id);
    snprintf_res(cmd_data, "CHIP ID: %s \n", chip_id);
    snprintf_res(cmd_data, "# current Software mac version: %d\n", ssv_root_version);
    snprintf_res(cmd_data, "COMPILER DATE %s \n", COMPILERDATE);
    SSV_GET_FW_VERSION(sc->sh, &regval);
    snprintf_res(cmd_data, "Firmware image version: %d\n", regval);
    return 0;
}
static int ssv_cmd_tool(struct ssv_softc *sc, int argc, char *argv[])
{
    u32 addr, value, count, len;
    char *endp;
    int s, retval;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    if ((argc == 4) && (strcmp(argv[1], "w") == 0)) {
        addr = simple_strtoul(argv[2], &endp, 16);
        value = simple_strtoul(argv[3], &endp, 16);
        if (SMAC_REG_WRITE(sc->sh, addr, value)) ;
        snprintf_res(cmd_data, "ok");
        return 0;
    }
    if (( (argc == 4) || (argc == 3)) && (strcmp(argv[1], "r") == 0)) {
        count = (argc==3)? 1: simple_strtoul(argv[3], &endp, 10);
        addr = simple_strtoul(argv[2], &endp, 16);
        for(s=0; s<count; s++, addr+=4) {
            if (SMAC_REG_READ(sc->sh, addr, &value)) ;
            snprintf_res(cmd_data, "%08x\n", value);
        }
        return 0;
    }
    if ((argc == 3) && (strcmp(argv[1], "auto_gen_nullpkt") == 0)) {
        value = simple_strtoul(argv[2], &endp, 10);
        retval = SSV_AUTO_GEN_NULLPKT(sc->sh, value);
        if (!retval)
            snprintf_res(cmd_data, "done to auto generate null frame\n");
        else
            snprintf_res(cmd_data, "Not suppout the tool\n");
        return 0;
    }
    if ((argc == 3) && (strcmp(argv[1], "dump_pbuf") == 0)) {
        value = simple_strtoul(argv[2], &endp, 10);
        addr = 0x80000000 | (value << 16);
        SMAC_REG_READ(sc->sh, addr, &len);
        len &= 0xffff;
        snprintf_res(cmd_data, "\n pbuf id=%d, len=%d\n\n", value, len);
        if (len > 1024)
            len = 1024;
        for(s=0; s<len; s+=4, addr+=4) {
            if (SMAC_REG_READ(sc->sh, addr, &value));
            if((s+4)%32 == 0)
                snprintf_res(cmd_data, " %08x\n", value);
            else
                snprintf_res(cmd_data, " %08x", value);
        }
        return 0;
    }
    return -1;
}
static int txtput_thread_m2(void *data)
{
#define Q_DELAY_MS 20
 struct sk_buff *skb = NULL;
 int qlen = 0, max_qlen, q_delay_urange[2];
 struct ssv_softc *sc = data;
 max_qlen = (200 * 1000 / 8 * Q_DELAY_MS) / sc->ssv_txtput.size_per_frame;
 q_delay_urange[0] = Q_DELAY_MS * 1000;
 q_delay_urange[1] = q_delay_urange[0] + 1000;
 printk("max_qlen: %d\n", max_qlen);
 while (!kthread_should_stop() && sc->ssv_txtput.loop_times > 0) {
  sc->ssv_txtput.loop_times--;
  skb = ssvdevice_skb_alloc(sc->ssv_txtput.size_per_frame);
  if (skb == NULL) {
   printk("ssv command txtput_generate_m2 "
   "ssvdevice_skb_alloc fail!!!\n");
   goto end;
  }
  skb->data_len = sc->ssv_txtput.size_per_frame ;
  skb->len = sc->ssv_txtput.size_per_frame ;
        SSV_TXTPUT_SET_DESC(sc->sh, skb);
  qlen = sc->sh->hci.hci_ops->hci_tx(sc->sh->hci.hci_ctrl, skb, 0, 0);
  if (qlen >= max_qlen) {
   usleep_range(q_delay_urange[0], q_delay_urange[1]);
  }
 }
end:
 sc->ssv_txtput.txtput_tsk = NULL;
 return 0;
}
int txtput_generate_m2(struct ssv_softc *sc, u32 size_per_frame, u32 loop_times)
{
 sc->ssv_txtput.size_per_frame = size_per_frame;
 sc->ssv_txtput.loop_times = loop_times;
 sc->ssv_txtput.txtput_tsk = kthread_run(txtput_thread_m2, sc, "txtput_thread_m2");
 return 0;
}
static int txtput_tsk_cleanup(struct ssv_softc *sc)
{
 int ret = 0;
 if (sc->ssv_txtput.txtput_tsk) {
  ret = kthread_stop(sc->ssv_txtput.txtput_tsk);
  sc->ssv_txtput.txtput_tsk = NULL;
 }
 return ret;
}
static int ssv_cmd_txtput(struct ssv_softc *sc, int argc, char *argv[])
{
 char *endp;
 u32 size_per_frame, loop_times;
 struct ssv_cmd_data *cmd_data = &sc->cmd_data;
 if ( (argc == 2) && (!strcmp(argv[1], "stop"))) {
  txtput_tsk_cleanup(sc);
  return 0;
 }
 if (argc != 3) {
  snprintf_res(cmd_data, "* txtput stop\n");
  snprintf_res(cmd_data, "* txtput [size] [frames]\n");
  snprintf_res(cmd_data, " EX: txtput 14000 9999 \n");
  return 0;
 }
 size_per_frame = simple_strtoul(argv[1], &endp, 10);
 loop_times = simple_strtoul(argv[2], &endp, 10);
 snprintf_res(cmd_data, "size & frames: %d & %d\n", size_per_frame, loop_times);
 if (sc->ssv_txtput.txtput_tsk) {
  snprintf_res(cmd_data, "txtput already in progress\n");
  return 0;
 }
 txtput_generate_m2(sc, size_per_frame + TXPB_OFFSET, loop_times);
 return 0;
}
#define MAX_FRM_SIZE 2304
static int ssv_cmd_rxtput(struct ssv_softc *sc, int argc, char *argv[])
{
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
 struct sdio_rxtput_cfg cmd_rxtput_cfg;
    char *endp;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    if (argc != 3) {
        snprintf_res(cmd_data, "rxtput [size] [frames]\n");
  return 0;
    }
 cmd_rxtput_cfg.size_per_frame = simple_strtoul(argv[1], &endp, 10);
 cmd_rxtput_cfg.total_frames = simple_strtoul(argv[2], &endp, 10);
 if (cmd_rxtput_cfg.size_per_frame > MAX_FRM_SIZE) {
     snprintf_res(cmd_data, "Frame size too large!!\n");
     return 0 ;
 }
    snprintf_res(cmd_data, "size & frames: %d& %d\n",
        cmd_rxtput_cfg.size_per_frame, cmd_rxtput_cfg.total_frames);
    skb = ssvdevice_skb_alloc(HOST_CMD_HDR_LEN + sizeof(struct sdio_rxtput_cfg));
    if(skb == NULL) {
        printk("ssv command ssvdevice_skb_alloc fail!!!\n");
        return 0;
    }
    skb->data_len = HOST_CMD_HDR_LEN + sizeof(struct sdio_rxtput_cfg);
    skb->len = skb->data_len;
    host_cmd = (struct cfg_host_cmd *)skb->data;
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RX_TPUT;
    host_cmd->len = skb->data_len;
    memcpy(host_cmd->dat32, &cmd_rxtput_cfg, sizeof(struct sdio_rxtput_cfg));
    if (sc->sh->hci.hci_ops->hci_send_cmd(sc->sh->hci.hci_ctrl, skb) == 0) {
        snprintf_res(cmd_data, "## hci cmd was sent successfully\n");
    } else {
        snprintf_res(cmd_data, "## hci cmd was sent failed\n");
    }
    ssvdevice_skb_free(skb);
 return 0;
}
static int ssv_cmd_check(struct ssv_softc *sc, int argc, char *argv[])
{
    u32 size,i,j,x,y,id,value,address,id_value;
    char *endp;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    u32 id_base_address[4];
#ifdef SSV_SUPPORT_HAL
#endif
    SSV_GET_RD_ID_ADR(sc->sh, &id_base_address[0]);
    if (argc != 2) {
        snprintf_res(cmd_data, "check [packet size]\n");
        return 0;
    }
    snprintf_res(cmd_data, " id address %x %x %x %x \n", id_base_address[0],
                 id_base_address[1], id_base_address[2], id_base_address[3]);
    size = simple_strtoul(argv[1], &endp, 10);
    size = size >> 2;
    for (x = 0; x < 4; x++) {
        if (SMAC_REG_READ(sc->sh, id_base_address[x], &id_value));
        for (y = 0; y < 32 && id_value; y++, id_value>>=1) {
            if (id_value & 0x1) {
                id = 32*x + y;
                address = 0x80000000 + (id << 16);
                {
                    printk("        ");
                    for (i = 0; i < size; i += 8){
                        if(SMAC_REG_READ(sc->sh, address, &value));
                        printk("\n%08X:%08X", address,value);
                        address += 4;
                        for (j = 1; j < 8; j++){
                            if(SMAC_REG_READ(sc->sh, address, &value));
                            printk(" %08X", value);
                            address += 4;
                        }
                    }
                    printk("\n");
                }
            }
        }
    }
    return 0;
}
static int ssv_cmd_rawpkt_context(char *buf, int len, void *file)
{
 struct file *fp = (struct file *)file;
 int rdlen;
 if (!file)
  return 0;
 rdlen = kernel_read(fp, fp->f_pos, buf, len);
 if (rdlen > 0)
  fp->f_pos += rdlen;
 return rdlen;
}
static void ssv_cmd_rawpkt_send(struct ssv_hw *sh, char *filename)
{
#define LEN_RAW_PACKET 15000
    struct sk_buff *skb = NULL;
    struct file *fp = NULL;
    u8 buffer[128];
    int len = 0;
    unsigned char *data = NULL;
    skb = ssvdevice_skb_alloc(LEN_RAW_PACKET);
    if (!skb)
        goto out;
    fp = filp_open(filename, O_RDONLY, 0);
    if (IS_ERR(fp))
        goto out;
    memset(buffer, 0x0, sizeof(buffer));
    while ((len = ssv_cmd_rawpkt_context((char*)buffer, sizeof(buffer), fp))) {
        data = skb_put(skb, len);
        memcpy(data, buffer, len);
    }
    HCI_SEND_CMD(sh, skb);
out:
    if (skb)
        ssvdevice_skb_free(skb);
    if (fp)
        filp_close(fp, NULL);
}
static int ssv_cmd_rawpkt(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    if (argc != 2) {
        snprintf_res(cmd_data, "\n rawpkt [binary file path] \n");
        return 0;
    }
    ssv_cmd_rawpkt_send(sc->sh, argv[1]);
    return 0;
}
static int ssv_cmd_directack(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    char *endp;
    if ((argc == 2) && (!strcmp(argv[1], "show"))) {
        snprintf_res(cmd_data, ">> act status = %s\n",
            ((sc->sc_flags & SC_OP_DIRECTLY_ACK) ? "direct complete" : "delay complete"));
        snprintf_res(cmd_data, ">> ampdu_tx_frame = %d\n", atomic_read(&sc->ampdu_tx_frame));
        snprintf_res(cmd_data, ">> directly_ack_low_threshold = %d\n", sc->directly_ack_low_threshold);
        snprintf_res(cmd_data, ">> directly_ack_high_threshold = %d\n", sc->directly_ack_high_threshold);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "low_threshold"))) {
        sc->directly_ack_low_threshold = simple_strtoul(argv[2], &endp, 10);
        snprintf_res(cmd_data, ">> Set directly_ack_low_threshold = %d\n", sc->directly_ack_low_threshold);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "high_threshold"))) {
        sc->directly_ack_high_threshold = simple_strtoul(argv[2], &endp, 10);
        snprintf_res(cmd_data, ">> Set directly_ack_high_threshold = %d\n", sc->directly_ack_high_threshold);
        return 0;
    } else {
        snprintf_res(cmd_data, "\n directack [show|low_threshold|high_threshold] [value] \n");
        return 0;
    }
    return 0;
}
static int ssv_cmd_txrxboost(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    char *endp;
    if ((argc == 2) && (!strcmp(argv[1], "show"))) {
        snprintf_res(cmd_data, ">> ampdu_tx_frame = %d\n", atomic_read(&sc->ampdu_tx_frame));
        snprintf_res(cmd_data, ">> txrxboost_prio = %d\n", sc->sh->cfg.txrxboost_prio);
        snprintf_res(cmd_data, ">> txrxboost_low_threshold = %d\n", sc->sh->cfg.txrxboost_low_threshold);
        snprintf_res(cmd_data, ">> txrxboost_high_threshold = %d\n", sc->sh->cfg.txrxboost_high_threshold);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "prio"))) {
        u32 prio = simple_strtoul(argv[2], &endp, 10);
        sc->sh->cfg.txrxboost_prio = (prio > (MAX_RT_PRIO-1))?(MAX_RT_PRIO-1):((prio < 0)?0:prio);
        snprintf_res(cmd_data, ">> Set txrxboost_prio = %d\n", sc->sh->cfg.txrxboost_prio);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "low_threshold"))) {
        sc->sh->cfg.txrxboost_low_threshold = simple_strtoul(argv[2], &endp, 10);
        snprintf_res(cmd_data, ">> Set txrxboost_low_threshold = %d\n", sc->sh->cfg.txrxboost_low_threshold);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "high_threshold"))) {
        sc->sh->cfg.txrxboost_high_threshold = simple_strtoul(argv[2], &endp, 10);
        snprintf_res(cmd_data, ">> Set txrxboost_high_threshold = %d\n", sc->sh->cfg.txrxboost_high_threshold);
        return 0;
    } else {
        snprintf_res(cmd_data, "\n txrxboost [show|prio|low_threshold|high_threshold] [value] \n");
        return 0;
    }
    return 0;
}
static int ssv_cmd_txrx_skb_q(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    struct ssv6xxx_hci_ctrl *hci_ctrl = sc->sh->hci.hci_ctrl;
    int txqid;
    char *endp;
    if ((argc == 2) && (!strcmp(argv[1], "show"))) {
        snprintf_res(cmd_data, ">> tx_skb_q = %u\n", skb_queue_len(&sc->tx_skb_q));
        snprintf_res(cmd_data, ">> rx_skb_q = %u\n", skb_queue_len(&sc->rx_skb_q));
        for(txqid=0; txqid<SSV_HW_TXQ_NUM; txqid++) {
            snprintf_res(cmd_data, ">> hw_txq[%d] = %u\n", txqid, skb_queue_len(&hci_ctrl->hw_txq[txqid].qhead));
        }
        snprintf_res(cmd_data, ">> rx_threshold = %d\n", sc->sh->cfg.rx_threshold);
        return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "rx_threshold"))) {
        sc->sh->cfg.rx_threshold = simple_strtoul(argv[2], &endp, 10);
        snprintf_res(cmd_data, ">> Set rx_threshold = %d\n", sc->sh->cfg.rx_threshold);
        return 0;
    } else {
        snprintf_res(cmd_data, "\n txrx_skb_q [show|rx_threshold] [value] \n");
        return 0;
    }
    return 0;
}
#ifdef SSV_SUPPORT_HAL
static int ssv_cmd_log(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    u32 log_hex = 0;
 int log_size = 0, total_log_size;
    if ((argc == 3) && (!strcmp(argv[1], "tx_desc"))) {
  if(!strcmp(argv[2], "enable")){
            sc->log_ctrl |= LOG_TX_DESC;
        } else {
            sc->log_ctrl &= ~(LOG_TX_DESC);
        }
        snprintf_res(cmd_data, "  log tx_desc %s\n", ((sc->log_ctrl & LOG_TX_DESC) ? "enable": "disable"));
    } else if ((argc == 3) && (!strcmp(argv[1], "rx_desc"))){
  if(!strcmp(argv[2], "enable")){
            sc->log_ctrl |= LOG_RX_DESC;
        } else {
            sc->log_ctrl &= ~(LOG_RX_DESC);
        }
        snprintf_res(cmd_data, "  log rx_desc %s\n", ((sc->log_ctrl & LOG_RX_DESC) ? "enable": "disable"));
    } else if ((argc == 3) && (!strcmp(argv[1], "tx_frame"))){
  if(!strcmp(argv[2], "enable")){
            sc->log_ctrl |= LOG_TX_FRAME;
        } else {
            sc->log_ctrl &= ~(LOG_TX_FRAME);
        }
        snprintf_res(cmd_data, "  log tx_frame %s\n", ((sc->log_ctrl & LOG_TX_FRAME) ? "enable": "disable"));
    } else if((argc == 4) && (!strcmp(argv[1], "ampdu"))){
  if (!strcmp(argv[2], "ssn")){
            if(!strcmp(argv[3], "enable")){
                sc->log_ctrl |= LOG_AMPDU_SSN;
            } else {
                sc->log_ctrl &= ~(LOG_AMPDU_SSN);
            }
         snprintf_res(cmd_data, "  log ampdu ssn %s\n", ((sc->log_ctrl & LOG_AMPDU_SSN) ? "enable": "disable"));
        } else if (!strcmp(argv[2], "dbg")){
            if(!strcmp(argv[3], "enable")){
                sc->log_ctrl |= LOG_AMPDU_DBG;
            } else {
                sc->log_ctrl &= ~(LOG_AMPDU_DBG);
            }
         snprintf_res(cmd_data, "  log ampdu dbg %s\n", ((sc->log_ctrl & LOG_AMPDU_DBG) ? "enable": "disable"));
        }else if (!strcmp(argv[2], "err")){
            if(!strcmp(argv[3], "enable")){
                sc->log_ctrl |= LOG_AMPDU_ERR;
            } else {
                sc->log_ctrl &= ~(LOG_AMPDU_ERR);
            }
         snprintf_res(cmd_data, "  log ampdu err %s\n", ((sc->log_ctrl & LOG_AMPDU_ERR) ? "enable": "disable"));
        }else{
            snprintf_res(cmd_data, " Invalid command!!\n");
            return 0;
        }
    }else if ((argc == 3) && (!strcmp(argv[1], "beacon"))) {
  if (!strcmp(argv[2], "enable")) {
            sc->log_ctrl |= LOG_BEACON;
        } else {
            sc->log_ctrl &= ~(LOG_BEACON);
        }
        snprintf_res(cmd_data, "  log beacon%s\n", ((sc->log_ctrl & LOG_BEACON) ? "enable": "disable"));
    } else if ((argc == 3) && (!strcmp(argv[1], "rate_control"))) {
    if (!strcmp(argv[2], "enable")) {
            sc->log_ctrl |= LOG_RATE_CONTROL;
        } else {
            sc->log_ctrl &= ~(LOG_RATE_CONTROL);
        }
        snprintf_res(cmd_data, "  log rate control %s\n", ((sc->log_ctrl & LOG_RATE_CONTROL) ? "enable": "disable"));
    } else if ((argc == 3) && (!strcmp(argv[1], "rate_report"))) {
     if (!strcmp(argv[2], "enable")) {
            sc->log_ctrl |= LOG_RATE_REPORT;
        } else {
            sc->log_ctrl &= ~(LOG_RATE_REPORT);
        }
        snprintf_res(cmd_data, "  log rate report %s\n", ((sc->log_ctrl & LOG_RATE_REPORT) ? "enable": "disable"));
 } else if ((argc == 3) && (!strcmp(argv[1], "hci"))){
        if(!strcmp(argv[2], "enable")){
            sc->log_ctrl |= LOG_HCI;
        } else {
            sc->log_ctrl &= ~(LOG_HCI);
        }
        snprintf_res(cmd_data, "  log hci %s\n", ((sc->log_ctrl & LOG_HCI) ? "enable": "disable"));
 } else if ((argc == 3) && (!strcmp(argv[1], "hwif"))){
        if(!strcmp(argv[2], "enable")){
            sc->log_ctrl |= LOG_HWIF;
   sc->sh->priv->dbg_control = true;
        } else {
            sc->log_ctrl &= ~(LOG_HWIF);
   sc->sh->priv->dbg_control = false;
        }
        snprintf_res(cmd_data, "  log hwif %s\n", ((sc->log_ctrl & LOG_HWIF) ? "enable": "disable"));
 } else if ((argc == 3) && (!strcmp(argv[1], "flash_bin"))){
        if(!strcmp(argv[2], "enable")){
            sc->log_ctrl |= LOG_FLASH_BIN;
        } else {
            sc->log_ctrl &= ~(LOG_FLASH_BIN);
        }
        snprintf_res(cmd_data, "  log flash_bin %s\n", ((sc->log_ctrl & LOG_FLASH_BIN) ? "enable": "disable"));
 } else if ((argc == 3) && (!strcmp(argv[1], "spectrum"))){
        if (!strcmp(argv[2], "enable")) {
            snprintf_res(cmd_data, "  Spectrum log is in kernel log\n");
            HAL_CMD_SPECTRUM(sc->sh);
        }
 } else if ((argc == 3) && (!strcmp(argv[1], "hex"))) {
  if (1 != sscanf(argv[2], "%x", &log_hex)) {
            snprintf_res(cmd_data, " log hex hexstring\n");
  } else {
   sc->log_ctrl = log_hex;
  }
        snprintf_res(cmd_data, "  log ctrl %d\n", sc->log_ctrl);
    } else if ((argc == 3) && (!strcmp(argv[1], "log_to_ram"))) {
  if (!strcmp(argv[2], "enable")) {
            sc->cmd_data.log_to_ram = true;
        } else {
            sc->cmd_data.log_to_ram = false;
        }
        snprintf_res(cmd_data, " log_to_ram %s\n", (sc->cmd_data.log_to_ram ? "enable" : "disable"));
  return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "ram_size"))) {
     if (1 == sscanf(argv[2], "%d", &log_size)) {
   if (sc->cmd_data.dbg_log.data) {
    kfree(sc->cmd_data.dbg_log.data);
    memset(&sc->cmd_data.dbg_log, 0, sizeof(struct ssv_dbg_log));
   }
   if (log_size != 0) {
    if (sc->cmd_data.dbg_log.data)
     kfree(sc->cmd_data.dbg_log.data);
    total_log_size = log_size * 1024;
    sc->cmd_data.dbg_log.data = (char *)kzalloc(total_log_size, GFP_KERNEL);
    if (sc->cmd_data.dbg_log.data == NULL) {
           snprintf_res(cmd_data, " Fail to alloc dbg_log_size %d Kbytes\n", log_size);
     return 0;
    }
    sc->cmd_data.dbg_log.size = 0;
    sc->cmd_data.dbg_log.totalsize = total_log_size;
    sc->cmd_data.dbg_log.top = sc->cmd_data.dbg_log.data;
    sc->cmd_data.dbg_log.tail = sc->cmd_data.dbg_log.data;
    sc->cmd_data.dbg_log.end = &(sc->cmd_data.dbg_log.data[total_log_size]);
   }
         snprintf_res(cmd_data, " alloc dbg_log_size %d Kbytes\n", log_size);
  } else {
         snprintf_res(cmd_data, " log ram_size [size] Kbytes\n");
  }
  return 0;
    } else if ((argc == 3) && (!strcmp(argv[1], "regw"))){
  if(!strcmp(argv[2], "enable")){
            sc->log_ctrl |= LOG_REGW;
        } else {
            sc->log_ctrl &= ~(LOG_REGW);
        }
        snprintf_res(cmd_data, "  log regw %s\n", ((sc->log_ctrl & LOG_REGW) ? "enable": "disable"));
 } else {
        snprintf_res(cmd_data, " log log_to_ram [enable | disable]\n");
        snprintf_res(cmd_data, " log ram_size [size] kb\n");
        snprintf_res(cmd_data, " log [category] [param] [enablel | disable]\n\n");
  snprintf_res(cmd_data, " category: tx_desc, tx_frame, rx_desc, ampdu, beacon\n");
  snprintf_res(cmd_data, "           rate_control, rate_report, hci, hwif, regw\n\n");
        snprintf_res(cmd_data, " ampdu param: ssn, dbg, err\n");
        return 0;
    }
    snprintf_res(cmd_data, "  log_ctrl 0x%x\n", sc->log_ctrl);
    return 0;
}
static int ssv_cmd_chan(struct ssv_softc *sc, int argc, char *argv[])
{
    char *endp;
    u32 ch;
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    struct ieee80211_supported_band *sband;
    struct ieee80211_channel *channel;
    enum nl80211_channel_type type = NL80211_CHAN_HT20;
    bool support_chan = false;
    int i, band;
    if ((argc == 2) || (argc == 3)) {
        if (argc == 2) {
            if (!strcmp(argv[1], "fixed") || !strcmp(argv[1], "auto")) {
                if (!strcmp(argv[1], "fixed"))
                    sc->sc_flags |= SC_OP_CHAN_FIXED;
                if (!strcmp(argv[1], "auto"))
                    sc->sc_flags &= ~SC_OP_CHAN_FIXED;
                snprintf_res(cmd_data, "\n %s channel fixed\n", ((sc->sc_flags & SC_OP_CHAN_FIXED) ? "Force" : "Clear"));
                return 0;
            }
        }
        ch = simple_strtoul(argv[1], &endp, 0);
        if (ch < 1) {
            snprintf_res(cmd_data, "\n  Channel syntax error.\n");
            return 0;
        }
        if (argc == 3) {
            if (!strcmp(argv[2], "bw40")) {
                if ((ch == 3) || (ch == 4) || (ch == 5) || (ch == 6) ||
                    (ch == 7) || (ch == 8) || (ch == 9) || (ch == 10) ||
                    (ch == 11) || (ch == 38) || (ch == 42) || (ch == 46) ||
                    (ch == 50) || (ch == 54) || (ch == 58) || (ch == 62) ||
                    (ch == 102) || (ch == 106) || (ch == 110) || (ch == 114) ||
                    (ch == 118) || (ch == 122) || (ch == 126) || (ch == 130) ||
                    (ch == 134) || (ch == 138) || (ch == 142) || (ch == 151) ||
                    (ch == 155) || (ch == 159)) {
                        type = NL80211_CHAN_HT40PLUS;
                        ch = ch - 2;
                } else {
                    snprintf_res(cmd_data, "\n  Channel syntax error.\n");
                    return 0;
                }
            } else if (!strcmp(argv[2], "+")) {
                if ((ch >= 8) && (ch <= 13)) {
                    snprintf_res(cmd_data, "\n  Channel syntax error.\n");
                    return 0;
                }
                type = NL80211_CHAN_HT40PLUS;
            } else if (!strcmp(argv[2], "-")) {
                if ((ch >= 1) && (ch <= 4)) {
                    snprintf_res(cmd_data, "\n  Channel syntax error.\n");
                    return 0;
                }
                type = NL80211_CHAN_HT40MINUS;
            } else {
                snprintf_res(cmd_data, "\n  Channel syntax error.\n");
                return 0;
            }
        }
        if (argc == 2)
            type = NL80211_CHAN_HT20;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
        for (band = 0; band < NUM_NL80211_BANDS; band++) {
#else
        for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
#endif
            if ((band == INDEX_80211_BAND_5GHZ) && !(sc->sh->cfg.hw_caps & SSV6200_HW_CAP_5GHZ))
                continue;
            sband = &sc->sbands[band];
            for (i = 0; i < sband->n_channels; i++) {
                channel = &sband->channels[i];
                if (ch == channel->hw_value)
                    support_chan = true;
            }
        }
        if (support_chan){
            struct ieee80211_channel chan;
            memset(&chan, 0 , sizeof( struct ieee80211_channel));
            chan.hw_value = ch;
            snprintf_res(cmd_data, "\n  switch to ch %d by command...\n", ch);
            HAL_SET_CHANNEL(sc, &chan, type);
            sc->hw_chan = chan.hw_value;
            sc->hw_chan_type = type;
            snprintf_res(cmd_data, "\n  DONE!!\n");
        }
        else
            snprintf_res(cmd_data, "\n  invalid ch %d\n", ch);
    } else {
        snprintf_res(cmd_data, "\n ch [chan_number] [chan_type]\n");
    }
    return 0;
}
static int ssv_cmd_cali(struct ssv_softc *sc, int argc, char *argv[])
{
    if((argc == 2) || (argc == 3)) {
        HAL_CMD_CALI(sc->sh, argc, argv);
    } else {
        snprintf_res(&sc->cmd_data,"\n cali [do|show|dpd(show |enable|disable)] \n");
    }
    return 0;
}
static int ssv_cmd_init(struct ssv_softc *sc, int argc, char *argv[])
{
    if (!strcmp(argv[1], "mac")) {
        SSV_PHY_ENABLE(sc->sh, false);
        HAL_INIT_MAC(sc->sh);
        SSV_PHY_ENABLE(sc->sh, true);
        snprintf_res(&sc->cmd_data, "\n   reload mac DONE\n");
    }else {
        snprintf_res(&sc->cmd_data, "\n init [mac]\n");
    }
    return 0;
}
static int ssv_cmd_rc(struct ssv_softc *sc, int argc, char *argv[])
{
    HAL_CMD_RC(sc->sh, argc, argv);
    return 0;
}
static int ssv_cmd_lpbk(struct ssv_softc *sc, int argc, char *argv[])
{
    HAL_CMD_LOOPBACK(sc->sh, argc, argv);
    return 0;
}
static int ssv_cmd_hwinfo(struct ssv_softc *sc, int argc, char *argv[])
{
    HAL_CMD_HWINFO(sc->sh, argc, argv);
    return 0;
}
static int ssv_cmd_cci(struct ssv_softc *sc, int argc, char *argv[])
{
    HAL_CMD_CCI(sc->sh, argc, argv);
    return 0;
}
static int ssv_cmd_txgen(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    u32 count = 0;
    if (argc == 2) {
        __string2u32(argv[1], &count, 0);
    } else if (argc == 1) {
        count = 1;
    } else {
        snprintf_res(cmd_data, "\n ./cli tx_gen [n] \n");
        return 0;
    }
    if (count != 0) {
       int i;
       for (i = 0; i < count; i++) {
           HAL_CMD_TXGEN(sc->sh);
           mdelay(1);
       }
    }
    snprintf_res(cmd_data, "\n tx_gen %d times done \n", count);
    return 0;
}
static int ssv_cmd_rf(struct ssv_softc *sc, int argc, char *argv[])
{
    HAL_CMD_RF(sc->sh, argc, argv);
    return 0;
}
static int ssv_cmd_hwq_limit(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    if ((argc == 3)&&(!strcmp(argv[1], "bk")||!strcmp(argv[1], "be")||!strcmp(argv[1], "vi")||!strcmp(argv[1], "vo")||!strcmp(argv[1], "mng"))) {
        HAL_CMD_HWQ_LIMIT(sc->sh, argc, argv);
    } else {
        snprintf_res(cmd_data, "%s [bk|be|vi|vo|mng] [queue limit]\n\n", argv[0]);
    }
    return 0;
}
static int ssv_cmd_efuse(struct ssv_softc *sc, int argc, char *argv[])
{
    HAL_CMD_EFUSE(sc->sh, argc, argv);
    return 0;
}
#endif
struct ssv_cmd_table cmd_table[] = {
    { "help", ssv_cmd_help, "ssv6200 command usage." , 2048},
    { "-h", ssv_cmd_help, "ssv6200 command usage." , 2048},
    { "--help", ssv_cmd_help, "ssv6200 command usage." , 2048},
    { "reg", ssv_cmd_reg, "ssv6200 register read/write." , 4096},
    { "cfg", ssv_cmd_cfg, "ssv6200 configuration." , 256},
    { "sta", ssv_cmd_sta, "svv6200 station info." , 4096},
    { "dump", ssv_cmd_dump, "dump ssv6200 tables." , 8192},
    { "hwq", ssv_cmd_hwq, "hardware queue staus" , 512},
#ifdef CONFIG_P2P_NOA
 { "noa", ssv_cmd_noa, "config noa param" , 512},
#endif
 { "irq", ssv_cmd_irq, "get sdio irq status." , 256},
    { "mac", ssv_cmd_mac, "ieee80211 swmac." , 256},
    { "hci", ssv_cmd_hci, "HCI command." , 1024},
    { "sdio", ssv_cmd_sdio, "SDIO command." , 128},
#if (defined (SSV_SUPPORT_HAL) || defined ( CONFIG_SSV_CABRIO_E))
    { "iqk", ssv_cmd_iqk, "iqk command" , 512},
#endif
    { "version",ssv_cmd_version,"version information" , 512},
    { "mib", ssv_cmd_mib, "mib counter related" , 2048},
    { "ps", ssv_cmd_power_saving, "power saving test" , 256},
    { "tool", ssv_cmd_tool, "ssv6200 tool register read/write." , 4096},
    { "rxtput", ssv_cmd_rxtput, "test rx sdio throughput" , 128},
    { "txtput", ssv_cmd_txtput, "test tx sdio throughput" , 256},
    { "check", ssv_cmd_check, "dump all allocate packet buffer" , 128},
    { "rawpkt", ssv_cmd_rawpkt, "send raw packet" , 512},
    { "directack", ssv_cmd_directack, "directly ack control" , 512},
    { "txrxboost", ssv_cmd_txrxboost, "tx/rx kernel thread priority boost" , 512},
    { "txrx_skb_q", ssv_cmd_txrx_skb_q, "tx/rx skb queue control" , 512},
#ifdef SSV_SUPPORT_HAL
    { "log", ssv_cmd_log, "enable debug log" , 256},
    { "ch", ssv_cmd_chan, "change channel by manual" , 128},
    { "cali", ssv_cmd_cali, "calibration for ssv6006" , 4096},
    { "init", ssv_cmd_init, "re-init " , 64},
    { "rc", ssv_cmd_rc, "fix rate set for ssv6006" , 4096},
    { "lpbk", ssv_cmd_lpbk, "lpbk test for ssv6006" , 4096},
 { "hwmsg", ssv_cmd_hwinfo, "hw message for ssv6006" , 4096},
 { "cci", ssv_cmd_cci, "turn on/off cci" , 128},
 { "txgen", ssv_cmd_txgen, "auto gen tx " , 128},
 { "rf", ssv_cmd_rf, "change parameters for rf tool" , 512},
 { "hwqlimit", ssv_cmd_hwq_limit, "Set software limit for hardware queue" , 512},
    { "efuse", ssv_cmd_efuse, "efuse read/write." , 2048},
#endif
    { NULL, NULL, NULL, 0},
};
int ssv_cmd_submit(struct ssv_cmd_data *cmd_data, char *cmd)
{
    struct ssv_cmd_table *sc_tbl;
    char *pch, ch;
    int ret, bf_size;
    char *sg_cmd_buffer;
    char *sg_argv[CLI_ARG_SIZE];
    u32 sg_argc;
    if (cmd_data->cmd_in_proc)
        return -1;
    sg_cmd_buffer = cmd;
    for (sg_argc = 0, ch = 0, pch = sg_cmd_buffer;
         (*pch!=0x00) && (sg_argc<CLI_ARG_SIZE); pch++ )
    {
        if ( (ch==0) && (*pch!=' ') )
        {
            ch = 1;
            sg_argv[sg_argc] = pch;
        }
        if ( (ch==1) && (*pch==' ') )
        {
            *pch = 0x00;
            ch = 0;
            sg_argc ++;
        }
    }
    if ( ch == 1)
    {
        sg_argc ++;
    }
    else if ( sg_argc > 0 )
    {
        *(pch-1) = ' ';
    }
    if ( sg_argc > 0 )
    {
        for( sc_tbl=cmd_table; sc_tbl->cmd; sc_tbl ++ )
        {
            if ( !strcmp(sg_argv[0], sc_tbl->cmd) )
            {
                struct ssv_softc *sc;
                struct ssv6xxx_hci_info *hci;
                sc = container_of(cmd_data, struct ssv_softc, cmd_data);
                hci = &sc->sh->hci;
                if ( (sc_tbl->cmd_func_ptr != ssv_cmd_cfg)
                    && (!hci->dev || !hci->if_ops || !sc->platform_dev)) {
                    cmd_data->ssv6xxx_result_buf = (char *)kzalloc(128, GFP_KERNEL);
                    if (!cmd_data->ssv6xxx_result_buf)
                         return -EFAULT;
                    cmd_data->ssv6xxx_result_buf[0] = 0x00;
     snprintf_res(cmd_data, "Member of ssv6xxx_ifdebug_info is NULL !\n");
     return -1;
    }
                cmd_data->ssv6xxx_result_buf = (char *)kzalloc(sc_tbl->result_buffer_size, GFP_KERNEL);
                if (!cmd_data->ssv6xxx_result_buf)
                    return -EFAULT;
                cmd_data->ssv6xxx_result_buf[0] = 0x00;
                cmd_data->rsbuf_len = 0;
                cmd_data->rsbuf_size = sc_tbl->result_buffer_size;
                cmd_data->cmd_in_proc = true;
                down_read(&sc->sta_info_sem);
                ret = sc_tbl->cmd_func_ptr(sc, sg_argc, sg_argv);
                up_read(&sc->sta_info_sem);
                if (ret < 0) {
                    strcpy(cmd_data->ssv6xxx_result_buf, "Invalid command !\n");
                }
                bf_size = strlen(cmd_data->ssv6xxx_result_buf);
                if ((sc_tbl->result_buffer_size -1) <= bf_size){
                    cmd_data->rsbuf_len = 0;
                    snprintf_res(cmd_data,
                       "\nALLOCATED BUFFER %d <= BUFFER USED +1 %d, OVERFLOW!!\n\n",
                       sc_tbl->result_buffer_size, bf_size+1);
                } else {
                    snprintf_res(cmd_data, "\nALLOCATED BUFFER %d , BUFFER USED %d\n\n",
                       sc_tbl->result_buffer_size, bf_size);
                }
                return 0;
            }
        }
        cmd_data->ssv6xxx_result_buf = (char *)kzalloc(64, GFP_KERNEL);
        if (!cmd_data->ssv6xxx_result_buf)
            return -EFAULT;
        cmd_data->ssv6xxx_result_buf[0] = 0x00;
        cmd_data->rsbuf_len = 0;
        cmd_data->rsbuf_size = 64;
        cmd_data->cmd_in_proc = true;
     snprintf_res(cmd_data, "Command not found !\n");
     return -EFAULT;
    } else {
        cmd_data->ssv6xxx_result_buf = (char *)kzalloc(64, GFP_KERNEL);
        if (!cmd_data->ssv6xxx_result_buf)
            return -EFAULT;
        cmd_data->ssv6xxx_result_buf[0] = 0x00;
        cmd_data->rsbuf_len = 0;
        cmd_data->rsbuf_size = 64;
        cmd_data->cmd_in_proc = true;
        snprintf_res(cmd_data, "./cli -h\n");
    }
    return 0;
}
