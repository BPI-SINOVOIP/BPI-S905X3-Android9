/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#ifndef __HWIF_H__
#define __HWIF_H__


#include <linux/mmc/host.h>
#include <linux/skbuff.h>
#include <ssv6xxx_common.h>
//#include <hwif/sdio/sdio_def.h>

// general adress for chip id
#define SYS_REG_BASE           0xc0000000

#define ADR_CHIP_ID_0                          (SYS_REG_BASE+0x00000008)          
#define ADR_CHIP_ID_1                          (SYS_REG_BASE+0x0000000c)          
#define ADR_CHIP_ID_2                          (SYS_REG_BASE+0x00000010)          
#define ADR_CHIP_ID_3                          (SYS_REG_BASE+0x00000014) 

#define SSVCABRIO_PLAT_EEP_MAX_WORDS	2048

/* Hardware Interface Property */
//bit0: hwif capability. (0: interrupt, 1:polling)
//bit1: interface.       (0: SDIO, 1:USB)
//mask define
#define SSV_HWIF_CAPABILITY_MASK       0x00000001
#define SSV_HWIF_INTERFACE_MASK        0x00000002
//shift define
#define SSV_HWIF_CAPABILITY_SFT        0
#define SSV_HWIF_INTERFACE_SFT         1
//capability
#define SSV_HWIF_CAPABILITY_INTERRUPT  (0 << SSV_HWIF_CAPABILITY_SFT)
#define SSV_HWIF_CAPABILITY_POLLING    (1 << SSV_HWIF_CAPABILITY_SFT)
//interface
#define SSV_HWIF_INTERFACE_SDIO        (0 << SSV_HWIF_INTERFACE_SFT)
#define SSV_HWIF_INTERFACE_USB         (1 << SSV_HWIF_INTERFACE_SFT)


/**
* Macros for turismo6200 register read/write access on Linux platform.
* @ SSV_REG_WRITE() : write 4-byte value into hardware register.
* @ SSV_REG_READ()  : read 4-byte value from hardware register.
*             
*/
#define SSV_REG_WRITE(dev, reg, val) \
        (sh)->priv->ops->writereg((sh)->sc->dev, (reg), (val))
#define SSV_REG_READ(dev, reg, buf)  \
        (sh)->priv->ops->readreg((sh)->sc->dev, (reg), (buf))

#define HWIF_DBG_PRINT(_pdata, format, args...)  \
    do { \
		if ((_pdata != NULL) && ((_pdata)->dbg_control)) \
			printk(format, ##args); \
    } while (0)

#if 0

/**
* Macros for turismo6200 register read/write access on Linux platform.
* @ SSV_REG_WRITE() : write 4-byte value into hardware register.
* @ SSV_REG_READ()  : read 4-byte value from hardware register.
* @ SSV_REG_SET_BITS: set the specified bits to a value.
*             
*/
#define SSV_REG_WRITE(sh, reg, val) \
        (sh)->priv->ops->writereg((sh)->sc->dev, (reg), (val))
#define SSV_REG_READ(sh, reg, buf)  \
        (sh)->priv->ops->readreg((sh)->sc->dev, (reg), (buf))
#define SSV_REG_CONFIRM(sh, reg, val)       \
{                                           \
    u32 regval;                             \
    SSV_REG_READ(sh, reg, &regval);         \
    if (regval != (val)) {                  \
        printk("[0x%08x]: 0x%08x!=0x%08x\n",\
        (reg), (val), regval);              \
        return -1;                          \
    }                                       \
}

#define SSV_REG_SET_BITS(sh, reg, set, clr) \
{                                           \
    u32 reg_val;                            \
    SSV_REG_READ(sh, reg, &reg_val);        \
    reg_val &= ~(clr);                      \
    reg_val |= (set);                       \
    SSV_REG_WRITE(sh, reg, reg_val);        \
}
#endif

struct sdio_scatter_req;
/**
* Hardware Interface (SDIO/SPI) APIs for turismo6200 on Linux platform.
*/
struct turismo_hwif_ops {
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
#ifdef CONFIG_PM
    int (*hwif_suspend)(struct device *child);
    int (*hwif_resume)(struct device *child);
#endif
    void (*hwif_cleanup)(struct device *child);
};

struct turismo_platform_data {
    u8                          chip_id[SSV6XXX_CHIP_ID_LENGTH+1];
    u8                          short_chip_id[SSV6XXX_CHIP_ID_SHORT_LENGTH+1];
    unsigned short              vendor;		/* vendor id */
    unsigned short              device;		/* device id */
};

#endif /* __HWIF_H__ */
