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

#ifndef __HWIF_H__
#define __HWIF_H__ 
#include <linux/mmc/host.h>
#include <ssv6xxx_common.h>
#define SYS_REG_BASE 0xc0000000
#define ADR_CHIP_ID_0 (SYS_REG_BASE+0x00000008)
#define ADR_CHIP_ID_1 (SYS_REG_BASE+0x0000000c)
#define ADR_CHIP_ID_2 (SYS_REG_BASE+0x00000010)
#define ADR_CHIP_ID_3 (SYS_REG_BASE+0x00000014)
#define SSVCABRIO_PLAT_EEP_MAX_WORDS 2048
#define SSV_HWIF_CAPABILITY_MASK 0x00000001
#define SSV_HWIF_INTERFACE_MASK 0x00000002
#define SSV_HWIF_CAPABILITY_SFT 0
#define SSV_HWIF_INTERFACE_SFT 1
#define SSV_HWIF_CAPABILITY_INTERRUPT (0 << SSV_HWIF_CAPABILITY_SFT)
#define SSV_HWIF_CAPABILITY_POLLING (1 << SSV_HWIF_CAPABILITY_SFT)
#define SSV_HWIF_INTERFACE_SDIO (0 << SSV_HWIF_INTERFACE_SFT)
#define SSV_HWIF_INTERFACE_USB (1 << SSV_HWIF_INTERFACE_SFT)
#define SSV_REG_WRITE(dev,reg,val) \
        (sh)->priv->ops->writereg((sh)->sc->dev, (reg), (val))
#define SSV_REG_READ(dev,reg,buf) \
        (sh)->priv->ops->readreg((sh)->sc->dev, (reg), (buf))
#define HWIF_DBG_PRINT(_pdata,format,args...) \
    do { \
  if ((_pdata != NULL) && ((_pdata)->dbg_control)) \
   printk(format, ##args); \
    } while (0)
#if 0
#define SSV_REG_WRITE(sh,reg,val) \
        (sh)->priv->ops->writereg((sh)->sc->dev, (reg), (val))
#define SSV_REG_READ(sh,reg,buf) \
        (sh)->priv->ops->readreg((sh)->sc->dev, (reg), (buf))
#define SSV_REG_CONFIRM(sh,reg,val) \
{ \
    u32 regval; \
    SSV_REG_READ(sh, reg, &regval); \
    if (regval != (val)) { \
        printk("[0x%08x]: 0x%08x!=0x%08x\n",\
        (reg), (val), regval); \
        return -1; \
    } \
}
#define SSV_REG_SET_BITS(sh,reg,set,clr) \
{ \
    u32 reg_val; \
    SSV_REG_READ(sh, reg, &reg_val); \
    reg_val &= ~(clr); \
    reg_val |= (set); \
    SSV_REG_WRITE(sh, reg, reg_val); \
}
#endif
struct sdio_scatter_req;
struct ssv6xxx_hwif_ops {
    int __must_check (*read)(struct device *child, void *buf,size_t *size, int mode);
    int __must_check (*write)(struct device *child, void *buf, size_t len,u8 queue_num);
    int __must_check (*readreg)(struct device *child, u32 addr, u32 *buf);
    int __must_check (*writereg)(struct device *child, u32 addr, u32 buf);
    int __must_check (*safe_readreg)(struct device *child, u32 addr, u32 *buf);
    int __must_check (*safe_writereg)(struct device *child, u32 addr, u32 buf);
    int __must_check (*burst_readreg)(struct device *child, u32 *addr, u32 *buf, u8 reg_amount);
    int __must_check (*burst_writereg)(struct device *child, u32 *addr, u32 *buf, u8 reg_amount);
    int __must_check (*burst_safe_readreg)(struct device *child, u32 *addr, u32 *buf, u8 reg_amount);
    int __must_check (*burst_safe_writereg)(struct device *child, u32 *addr, u32 *buf, u8 reg_amount);
    int (*trigger_tx_rx)(struct device *child);
    int (*irq_getmask)(struct device *child, u32 *mask);
    void (*irq_setmask)(struct device *child,int mask);
    void (*irq_enable)(struct device *child);
    void (*irq_disable)(struct device *child,bool iswaitirq);
    int (*irq_getstatus)(struct device *child,int *status);
    void (*irq_request)(struct device *child,irq_handler_t irq_handler,void *irq_dev);
    void (*irq_trigger)(struct device *child);
    void (*pmu_wakeup)(struct device *child);
    int __must_check (*load_fw)(struct device *child, u32 start_addr, u8 *data, int data_length);
    void (*load_fw_pre_config_device)(struct device *child);
    void (*load_fw_post_config_device)(struct device *child);
    int (*cmd52_read)(struct device *child, u32 addr, u32 *value);
    int (*cmd52_write)(struct device *child, u32 addr, u32 value);
    bool (*support_scatter)(struct device *child);
    int (*rw_scatter)(struct device *child, struct sdio_scatter_req *scat_req);
    bool (*is_ready)(struct device *child);
    int (*write_sram)(struct device *child, u32 addr, u8 *data, u32 size);
    void (*interface_reset)(struct device *child);
    int (*start_usb_acc)(struct device *child, u8 epnum);
    int (*stop_usb_acc)(struct device *child, u8 epnum);
    int (*jump_to_rom)(struct device *child);
    int (*property)(struct device *child);
    void (*sysplf_reset)(struct device *child, u32 addr, u32 value);
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
    void (*hwif_rx_task)(struct device *child, int (*rx_cb)(struct sk_buff_head *rxq, void *args), int (*is_rx_q_full)(void *args), void *args, u32 *pkt);
#else
    void (*hwif_rx_task)(struct device *child, int (*rx_cb)(struct sk_buff *rx_skb, void *args), int (*is_rx_q_full)(void *args), void *args, u32 *pkt);
#endif
};
struct ssv6xxx_platform_data {
    atomic_t irq_handling;
    bool is_enabled;
    u8 chip_id[SSV6XXX_CHIP_ID_LENGTH];
    u8 short_chip_id[SSV6XXX_CHIP_ID_SHORT_LENGTH+1];
    unsigned short vendor;
    unsigned short device;
    struct ssv6xxx_hwif_ops *ops;
 bool dbg_control;
    struct sk_buff *(*skb_alloc) (void *param, s32 len, gfp_t gfp_mask);
    void (*skb_free) (void *param, struct sk_buff *skb);
    void *skb_param;
#ifdef CONFIG_PM
 void (*suspend)(void *param);
 void (*resume)(void *param);
 void *pm_param;
#endif
    void (*enable_usb_acc)(void *param, u8 epnum);
    void (*disable_usb_acc)(void *param, u8 epnum);
    void (*jump_to_rom)(void *param);
    void *usb_param;
    int (*rx_burstread_size)(void *param);
    void *rx_burstread_param;
};
#endif
