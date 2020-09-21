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

#include <linux/version.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
#include <linux/printk.h>
#else
#include <linux/kernel.h>
#endif
#include <hwif/hwif.h>
#include "sdio_def.h"
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
#include "sdio.h"
#endif
#define LOW_SPEED_SDIO_CLOCK (25000000)
#define HIGH_SPEED_SDIO_CLOCK (50000000)
#define MAX_RX_FRAME_SIZE 0x900
#define MAX_REG_RETRY_CNT (3)
#define SSV_VENDOR_ID 0x3030
#define SSV_CABRIO_DEVID 0x3030
#define CHECK_IO_RET(GLUE,RET) \
    do { \
        if (RET) { \
            if ((++((GLUE)->err_count)) > MAX_ERR_COUNT) \
                printk(KERN_ERR "MAX SDIO Error\n"); \
        } else \
            (GLUE)->err_count = 0; \
    } while (0)
#define MAX_ERR_COUNT (10)
struct ssv6xxx_sdio_glue
{
    struct device *dev;
    struct platform_device *core;
    struct ssv6xxx_platform_data *p_wlan_data;
    struct ssv6xxx_platform_data  tmp_data;
#ifdef CONFIG_MMC_DISALLOW_STACK
    PLATFORM_DMA_ALIGNED u8 rreg_data[4];
    PLATFORM_DMA_ALIGNED u8 wreg_data[8];
    PLATFORM_DMA_ALIGNED u32 brreg_data[MAX_BURST_READ_REG_AMOUNT];
    PLATFORM_DMA_ALIGNED u8 bwreg_data[MAX_BURST_WRITE_REG_AMOUNT][8];
    PLATFORM_DMA_ALIGNED u32 aggr_readsz;
#endif
#ifdef CONFIG_FW_ALIGNMENT_CHECK
    struct sk_buff *dmaSkb;
#endif


    /* for ssv SDIO */
    unsigned int                dataIOPort;
    unsigned int                regIOPort;

    irq_handler_t               irq_handler;
    void *irq_dev;
    bool                        dev_ready;
    unsigned int                err_count;
};
static void ssv6xxx_high_sdio_clk(struct sdio_func *func);
static void ssv6xxx_low_sdio_clk(struct sdio_func *func);
static void ssv6xxx_do_sdio_reset_reinit(struct ssv6xxx_platform_data *pwlan_data,
        struct sdio_func *func, struct ssv6xxx_sdio_glue *glue);
#define IS_GLUE_INVALID(glue) \
      ( (glue == NULL) \
       || (glue->dev_ready == false) \
       || ( (glue->p_wlan_data != NULL) \
           && (glue->p_wlan_data->is_enabled == false)) \
       || (glue->err_count > MAX_ERR_COUNT))
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
static const struct sdio_device_id ssv6xxx_sdio_devices[] __devinitconst =
#else
static const struct sdio_device_id ssv6xxx_sdio_devices[] =
#endif
{
#if 0
    { SDIO_DEVICE(SSV_VENDOR_ID, SSV_CABRIO_DEVID) },
#endif
    {}
};
MODULE_DEVICE_TABLE(sdio, ssv6xxx_sdio_devices);
static bool ssv6xxx_is_ready (struct device *child)
{
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    if (IS_GLUE_INVALID(glue))
        return false;
    return glue->dev_ready;
}
static int ssv6xxx_sdio_cmd52_read(struct device *child, u32 addr,
        u32 *value)
{
    int ret = -1;
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
    if (IS_GLUE_INVALID(glue))
  return ret;
    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        *value = sdio_readb(func, addr, &ret);
        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
    }
    return ret;
}
static int ssv6xxx_sdio_cmd52_write(struct device *child, u32 addr,
        u32 value)
{
    int ret = -1;
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
    if (IS_GLUE_INVALID(glue)){
 		 return ret;
	}
    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        sdio_writeb(func, value, addr, &ret);
        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
    }
    return ret;
}
static int __must_check __ssv6xxx_sdio_read_reg (struct ssv6xxx_sdio_glue *glue, u32 addr,
                                                 u32 *buf)
{
    int ret = (-1);
    struct sdio_func *func ;
#if !defined(CONFIG_MMC_DISALLOW_STACK) && defined(CONFIG_FW_ALIGNMENT_CHECK)
    PLATFORM_DMA_ALIGNED u8 data[4];
#elif !defined(CONFIG_MMC_DISALLOW_STACK)
    u8 data[4];
#endif

    if (IS_GLUE_INVALID(glue)){
		return ret;
    }
   
    //dev_err(&func->dev, "sdio read reg device[%08x] parent[%08x]\n",child,child->parent);

    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);

        // 4 bytes address
#ifdef CONFIG_MMC_DISALLOW_STACK
        glue->rreg_data[0] = (addr >> ( 0 )) &0xff;
        glue->rreg_data[1] = (addr >> ( 8 )) &0xff;
        glue->rreg_data[2] = (addr >> ( 16 )) &0xff;
        glue->rreg_data[3] = (addr >> ( 24 )) &0xff;
#else
        data[0] = (addr >> ( 0 )) &0xff;
        data[1] = (addr >> ( 8 )) &0xff;
        data[2] = (addr >> ( 16 )) &0xff;
        data[3] = (addr >> ( 24 )) &0xff;
#endif
#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_toio(func, glue->regIOPort, glue->rreg_data, 4);
#else
        ret = sdio_memcpy_toio(func, glue->regIOPort, data, 4);
#endif
        if (WARN_ON(ret))
        {
            dev_err(&func->dev, "sdio read reg write address failed (%d)\n", ret);
            goto io_err;
        }

#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_fromio(func, glue->rreg_data, glue->regIOPort, 4);
#else
        ret = sdio_memcpy_fromio(func, data, glue->regIOPort, 4);
#endif
        if (WARN_ON(ret))
        {
            dev_err(&func->dev, "sdio read reg from I/O failed (%d)\n",ret);
         goto io_err;
      }
        if(ret == 0)
        {
#ifdef CONFIG_MMC_DISALLOW_STACK
            *buf = (glue->rreg_data[0]&0xff);
            *buf = *buf | ((glue->rreg_data[1]&0xff)<<( 8 ));
            *buf = *buf | ((glue->rreg_data[2]&0xff)<<( 16 ));
            *buf = *buf | ((glue->rreg_data[3]&0xff)<<( 24 ));
#else
            *buf = (data[0]&0xff);
            *buf = *buf | ((data[1]&0xff)<<( 8 ));
            *buf = *buf | ((data[2]&0xff)<<( 16 ));
            *buf = *buf | ((data[3]&0xff)<<( 24 ));
#endif
        }
        else
            *buf = 0xffffffff;
io_err:
        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
    }
    else
    {
        dev_err(&func->dev, "sdio read reg glue == NULL!!!\n");
    }
    return ret;
}
static int __must_check __ssv6xxx_sdio_safe_read_reg (struct ssv6xxx_sdio_glue *glue, u32 addr,
                                                 u32 *buf)
{
    int ret = (-1), rdy_flag_cnt = 0;
    struct sdio_func *func ;
#if !defined(CONFIG_MMC_DISALLOW_STACK) && defined(CONFIG_FW_ALIGNMENT_CHECK)
    PLATFORM_DMA_ALIGNED u8 data[4];
#elif !defined(CONFIG_MMC_DISALLOW_STACK)
    u8 data[4];
#endif

    if (IS_GLUE_INVALID(glue)){
		return ret;
    }
   
    //dev_err(&func->dev, "sdio read reg device[%08x] parent[%08x]\n",child,child->parent);

    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);

        // 4 bytes address
#ifdef CONFIG_MMC_DISALLOW_STACK
        glue->rreg_data[0] = (addr >> ( 0 )) &0xff;
        glue->rreg_data[1] = (addr >> ( 8 )) &0xff;
        glue->rreg_data[2] = (addr >> ( 16 )) &0xff;
        glue->rreg_data[3] = (addr >> ( 24 )) &0xff;
#else
        data[0] = (addr >> ( 0 )) &0xff;
        data[1] = (addr >> ( 8 )) &0xff;
        data[2] = (addr >> ( 16 )) &0xff;
        data[3] = (addr >> ( 24 )) &0xff;
#endif

        //8 byte ( 4 bytes address , 4 bytes data )
#if (defined(SSV_SUPPORT_SSV6006))
        while(sdio_readb(func, REG_SD_READY_FLAG, &ret) != SDIO_READY_FLAG_IDLE) {
            if (ret != 0) {
                printk("%s: ret=%d", __func__, ret);
                goto io_err;
            } else if (++rdy_flag_cnt > SDIO_READY_FLAG_BUSY_THRESHOLD) {
                ret = -EBUSY;
                dev_err(&func->dev, "%s: bus is busy\n", __func__);
                goto io_err;
            }
            udelay(SDIO_READY_FLAG_BUSY_DELAY);
        }
#endif
#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_toio(func, glue->regIOPort, glue->rreg_data, 4);
#else
        ret = sdio_memcpy_toio(func, glue->regIOPort, data, 4);
#endif
        if (WARN_ON(ret))
        {
            dev_err(&func->dev, "%s: sdio write to I/O failed (%d)\n", __func__, ret);
            goto io_err;
        }
        rdy_flag_cnt = 0;
#if (defined(SSV_SUPPORT_SSV6006))
        while(sdio_readb(func, REG_SD_READY_FLAG, &ret) != SDIO_READY_FLAG_IDLE) {
            if (ret != 0) {
                printk("%s: ret=%d", __func__, ret);
                goto io_err;
            } else if (++rdy_flag_cnt > SDIO_READY_FLAG_BUSY_THRESHOLD) {
                ret = -EBUSY;
                dev_err(&func->dev, "%s: bus is busy\n", __func__);
                goto io_err;
            }
            udelay(SDIO_READY_FLAG_BUSY_DELAY);
        }
#endif

#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_fromio(func, glue->rreg_data, glue->regIOPort, 4);
#else
        ret = sdio_memcpy_fromio(func, data, glue->regIOPort, 4);
#endif
        if (WARN_ON(ret))
        {
            dev_err(&func->dev, "%s: sdio read from I/O failed (%d)\n", __func__,ret);
         goto io_err;
      }
        if(ret == 0)
        {
#ifdef CONFIG_MMC_DISALLOW_STACK
            *buf = (glue->rreg_data[0]&0xff);
            *buf = *buf | ((glue->rreg_data[1]&0xff)<<( 8 ));
            *buf = *buf | ((glue->rreg_data[2]&0xff)<<( 16 ));
            *buf = *buf | ((glue->rreg_data[3]&0xff)<<( 24 ));
#else
            *buf = (data[0]&0xff);
            *buf = *buf | ((data[1]&0xff)<<( 8 ));
            *buf = *buf | ((data[2]&0xff)<<( 16 ));
            *buf = *buf | ((data[3]&0xff)<<( 24 ));
#endif
        }
        else
            *buf = 0xffffffff;
io_err:
        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
    }
    else
    {
        dev_err(&func->dev, "sdio read reg glue == NULL!!!\n");
    }
    return ret;
}
static int __must_check ssv6xxx_sdio_read_reg(struct device *child, u32 addr,
        u32 *buf)
{
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
 int i, ret;
 for (i = 0; i < MAX_REG_RETRY_CNT; i++) {
  ret = __ssv6xxx_sdio_read_reg(glue, addr, buf);
  if (!ret)
   return ret;
 }
 HWIF_DBG_PRINT(glue->p_wlan_data, "%s: Fail to read register, addr 0x%08x\n", __FUNCTION__, addr);
    return ret;
}
static int __must_check ssv6xxx_sdio_safe_read_reg(struct device *child, u32 addr,
        u32 *buf)
{
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
 int i, ret;
 for (i = 0; i < MAX_REG_RETRY_CNT; i++) {
  ret = __ssv6xxx_sdio_safe_read_reg(glue, addr, buf);
  if (!ret)
   return ret;
 }
 HWIF_DBG_PRINT(glue->p_wlan_data, "%s: Fail to read register, addr 0x%08x\n", __FUNCTION__, addr);
    return ret;
}
#ifdef ENABLE_WAKE_IO_ISR_WHEN_HCI_ENQUEUE
static int ssv6xxx_sdio_trigger_tx_rx (struct device *child)
{
    int ret = (-1);
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
    struct mmc_host *host;
    if (IS_GLUE_INVALID(glue))
        return ret;
    func = dev_to_sdio_func(glue->dev);
    host = func->card->host;
    mmc_signal_sdio_irq(host);
    return 0;
}
#endif
static int __must_check __ssv6xxx_sdio_write_reg (struct ssv6xxx_sdio_glue *glue, u32 addr,
                                                  u32 buf)
{
    int ret = (-1);
    struct sdio_func *func;
#if !defined(CONFIG_MMC_DISALLOW_STACK) && defined(CONFIG_FW_ALIGNMENT_CHECK)
    PLATFORM_DMA_ALIGNED u8 data[8];
#elif !defined(CONFIG_MMC_DISALLOW_STACK)
    u8 data[8];
#endif

    if (IS_GLUE_INVALID(glue)){
        return ret;
    }

    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);

        // 4 bytes address
#ifdef CONFIG_MMC_DISALLOW_STACK
        glue->wreg_data[0] = (addr >> ( 0 )) &0xff;
        glue->wreg_data[1] = (addr >> ( 8 )) &0xff;
        glue->wreg_data[2] = (addr >> ( 16 )) &0xff;
        glue->wreg_data[3] = (addr >> ( 24 )) &0xff;
#else
        data[0] = (addr >> ( 0 )) &0xff;
        data[1] = (addr >> ( 8 )) &0xff;
        data[2] = (addr >> ( 16 )) &0xff;
        data[3] = (addr >> ( 24 )) &0xff;
#endif

        // 4 bytes data
#ifdef CONFIG_MMC_DISALLOW_STACK
        glue->wreg_data[4] = (buf >> ( 0 )) &0xff;
        glue->wreg_data[5] = (buf >> ( 8 )) &0xff;
        glue->wreg_data[6] = (buf >> ( 16 )) &0xff;
        glue->wreg_data[7] = (buf >> ( 24 )) &0xff;
#else
        data[4] = (buf >> ( 0 )) &0xff;
        data[5] = (buf >> ( 8 )) &0xff;
        data[6] = (buf >> ( 16 )) &0xff;
        data[7] = (buf >> ( 24 )) &0xff;
#endif

        //8 byte ( 4 bytes address , 4 bytes data )
#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_toio(func, glue->regIOPort, glue->wreg_data, 8);
#else
        ret = sdio_memcpy_toio(func, glue->regIOPort, data, 8);
#endif

        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
    }
    else
    {
        dev_err(&func->dev, "sdio write reg glue == NULL!!!\n");
    }
    return ret;
}
static int __must_check __ssv6xxx_sdio_safe_write_reg (struct ssv6xxx_sdio_glue *glue, u32 addr,
                                                  u32 buf)
{
    int ret = (-1);
    struct sdio_func *func;
#if !defined(CONFIG_MMC_DISALLOW_STACK) && defined(CONFIG_FW_ALIGNMENT_CHECK)
    PLATFORM_DMA_ALIGNED u8 data[8];
#elif !defined(CONFIG_MMC_DISALLOW_STACK)
    u8 data[8];
#endif
#if (defined(SSV_SUPPORT_SSV6006))
    int rdy_flag_cnt = 0;
#endif
    if (IS_GLUE_INVALID(glue))
        return ret;
    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
#ifdef CONFIG_MMC_DISALLOW_STACK
        glue->wreg_data[0] = (addr >> ( 0 )) &0xff;
        glue->wreg_data[1] = (addr >> ( 8 )) &0xff;
        glue->wreg_data[2] = (addr >> ( 16 )) &0xff;
        glue->wreg_data[3] = (addr >> ( 24 )) &0xff;
#else
        data[0] = (addr >> ( 0 )) &0xff;
        data[1] = (addr >> ( 8 )) &0xff;
        data[2] = (addr >> ( 16 )) &0xff;
        data[3] = (addr >> ( 24 )) &0xff;
#endif
#ifdef CONFIG_MMC_DISALLOW_STACK
        glue->wreg_data[4] = (buf >> ( 0 )) &0xff;
        glue->wreg_data[5] = (buf >> ( 8 )) &0xff;
        glue->wreg_data[6] = (buf >> ( 16 )) &0xff;
        glue->wreg_data[7] = (buf >> ( 24 )) &0xff;
#else
        data[4] = (buf >> ( 0 )) &0xff;
        data[5] = (buf >> ( 8 )) &0xff;
        data[6] = (buf >> ( 16 )) &0xff;
        data[7] = (buf >> ( 24 )) &0xff;
#endif

#if (defined(SSV_SUPPORT_SSV6006))
        while(sdio_readb(func, REG_SD_READY_FLAG, &ret) != SDIO_READY_FLAG_IDLE) {
            if (ret != 0) {
                printk("%s: ret=%d", __func__, ret);
                goto io_err;
            } else if (++rdy_flag_cnt > SDIO_READY_FLAG_BUSY_THRESHOLD) {
                ret = -EBUSY;
                dev_err(&func->dev, "%s: bus is busy\n", __func__);
                goto io_err;
            }
            udelay(SDIO_READY_FLAG_BUSY_DELAY);
        }
#endif
#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_toio(func, glue->regIOPort, glue->wreg_data, 8);
#else
        ret = sdio_memcpy_toio(func, glue->regIOPort, data, 8);
#endif
        if (WARN_ON(ret))
        {
            dev_err(&func->dev, "%s: sdio write to I/O failed (%d)\n", __func__, ret);
            goto io_err;
        }
#if (defined(SSV_SUPPORT_SSV6006))
        while(sdio_readb(func, REG_SD_READY_FLAG, &ret) != SDIO_READY_FLAG_IDLE) {
            if (ret != 0) {
                printk("%s: ret=%d", __func__, ret);
                goto io_err;
            } else if (++rdy_flag_cnt > SDIO_READY_FLAG_BUSY_THRESHOLD) {
                ret = -EBUSY;
                dev_err(&func->dev, "%s: bus is busy\n", __func__);
                goto io_err;
            }
            udelay(SDIO_READY_FLAG_BUSY_DELAY);
        }
#endif
io_err:
        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
    }
    else
    {
        dev_err(&func->dev, "sdio write reg glue == NULL!!!\n");
    }
    return ret;
}
static int __must_check ssv6xxx_sdio_write_reg(struct device *child, u32 addr,
        u32 buf)
{
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    int i, ret;
 for (i = 0; i < MAX_REG_RETRY_CNT; i++) {
  ret = __ssv6xxx_sdio_write_reg(glue, addr, buf);
  if (!ret){
#ifdef __x86_64
   udelay(50);
#endif
   return ret;
     }
 }
 HWIF_DBG_PRINT(glue->p_wlan_data, "%s: Fail to write register, addr 0x%08x, value 0x%08x\n", __FUNCTION__, addr, buf);
    return ret;
}
static int __must_check ssv6xxx_sdio_safe_write_reg(struct device *child, u32 addr,
        u32 buf)
{
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    int i, ret;
 for (i = 0; i < MAX_REG_RETRY_CNT; i++) {
  ret = __ssv6xxx_sdio_safe_write_reg(glue, addr, buf);
  if (!ret){
#ifdef __x86_64
   udelay(50);
#endif
   return ret;
     }
 }
 HWIF_DBG_PRINT(glue->p_wlan_data, "%s: Fail to write register, addr 0x%08x, value 0x%08x\n", __FUNCTION__, addr, buf);
    return ret;
}
static int __must_check ssv6xxx_sdio_burst_read_reg(struct device *child, u32 *addr,
        u32 *buf, u8 reg_amount)
{
    int ret = (-1);
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func ;
#if !defined(CONFIG_MMC_DISALLOW_STACK) && defined(CONFIG_FW_ALIGNMENT_CHECK)
    PLATFORM_DMA_ALIGNED u32 data[MAX_BURST_READ_REG_AMOUNT]={0};
#elif !defined(CONFIG_MMC_DISALLOW_STACK)
    u32 data[MAX_BURST_READ_REG_AMOUNT]={0};
#endif
    u8 i = 0;

    if (IS_GLUE_INVALID(glue)){
        return ret;
    }

    if (reg_amount > MAX_BURST_READ_REG_AMOUNT)
    {
        HWIF_DBG_PRINT(glue->p_wlan_data, "The amount of sdio burst-read register must <= %d\n",
    MAX_BURST_READ_REG_AMOUNT);
        return ret;
    }
    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        for (i=0; i<reg_amount; i++)
        {
#ifdef CONFIG_MMC_DISALLOW_STACK
            memcpy(&glue->brreg_data[i], &addr[i], 4);
#else
            memcpy(&data[i], &addr[i], 4);
#endif
        }        

#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_toio(func, IO_REG_BURST_RD_PORT_REG, glue->brreg_data, reg_amount*4);
#else
        ret = sdio_memcpy_toio(func, IO_REG_BURST_RD_PORT_REG, data, reg_amount*4);
#endif

        if (WARN_ON(ret))
        {
            dev_err(child->parent, "sdio burst-read reg write address failed (%d)\n", ret);
            goto io_err;
        }        

#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_fromio(func, glue->brreg_data, IO_REG_BURST_RD_PORT_REG, reg_amount*4);
#else
        ret = sdio_memcpy_fromio(func, data, IO_REG_BURST_RD_PORT_REG, reg_amount*4);
#endif
        if (WARN_ON(ret))
        {
            dev_err(child->parent, "sdio burst-read reg from I/O failed (%d)\n",ret);
         goto io_err;
        }
        if(ret == 0)
#ifdef CONFIG_MMC_DISALLOW_STACK
            memcpy(buf, glue->brreg_data, reg_amount*4);
#else
            memcpy(buf, data, reg_amount*4);
#endif
        else
            memset(buf, 0xffffffff, reg_amount*4);
io_err:
        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
    }
    else
    {
        dev_err(child->parent, "sdio burst-read reg glue == NULL!!!\n");
    }
    return ret;
}
static int __must_check ssv6xxx_sdio_burst_safe_read_reg(struct device *child, u32 *addr,
        u32 *buf, u8 reg_amount)
{
    int ret = (-1);
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func ;
#if !defined(CONFIG_MMC_DISALLOW_STACK) && defined(CONFIG_FW_ALIGNMENT_CHECK)
    PLATFORM_DMA_ALIGNED u32 data[MAX_BURST_READ_REG_AMOUNT]={0};
#elif !defined(CONFIG_MMC_DISALLOW_STACK)
    u32 data[MAX_BURST_READ_REG_AMOUNT]={0};
#endif
    u8 i = 0;
#if (defined(SSV_SUPPORT_SSV6006))
    int rdy_flag_cnt = 0;
#endif
    if (IS_GLUE_INVALID(glue)){
        return ret;
    }

    if (reg_amount > MAX_BURST_READ_REG_AMOUNT)
    {
        HWIF_DBG_PRINT(glue->p_wlan_data, "The amount of sdio burst-read register must <= %d\n",
    MAX_BURST_READ_REG_AMOUNT);
        return ret;
    }
    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        for (i=0; i<reg_amount; i++)
        {
#ifdef CONFIG_MMC_DISALLOW_STACK
            memcpy(&glue->brreg_data[i], &addr[i], 4);
#else
            memcpy(&data[i], &addr[i], 4);
#endif
        }        

#if (defined(SSV_SUPPORT_SSV6006))
        while(sdio_readb(func, REG_SD_READY_FLAG, &ret) != SDIO_READY_FLAG_IDLE) {
            if (ret != 0) {
                printk("%s: ret=%d", __func__, ret);
                goto io_err;
            } else if (++rdy_flag_cnt > SDIO_READY_FLAG_BUSY_THRESHOLD) {
                ret = -EBUSY;
                dev_err(&func->dev, "%s: bus is busy\n", __func__);
                goto io_err;
            }
            udelay(SDIO_READY_FLAG_BUSY_DELAY);
        }
#endif

#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_toio(func, IO_REG_BURST_RD_PORT_REG, glue->brreg_data, reg_amount*4);
#else
        ret = sdio_memcpy_toio(func, IO_REG_BURST_RD_PORT_REG, data, reg_amount*4);
#endif

        if (WARN_ON(ret))
        {
            dev_err(child->parent, "%s: sdio write to I/O failed (%d)\n", __func__, ret);
            goto io_err;
        }
#if (defined(SSV_SUPPORT_SSV6006))
        while(sdio_readb(func, REG_SD_READY_FLAG, &ret) != SDIO_READY_FLAG_IDLE) {
            if (ret != 0) {
                printk("%s: ret=%d", __func__, ret);
                goto io_err;
            } else if (++rdy_flag_cnt > SDIO_READY_FLAG_BUSY_THRESHOLD) {
                ret = -EBUSY;
                dev_err(&func->dev, "%s: bus is busy\n", __func__);
                goto io_err;
            }
            udelay(SDIO_READY_FLAG_BUSY_DELAY);
        }
#endif

#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_fromio(func, glue->brreg_data, IO_REG_BURST_RD_PORT_REG, reg_amount*4);
#else
        ret = sdio_memcpy_fromio(func, data, IO_REG_BURST_RD_PORT_REG, reg_amount*4);
#endif
        if (WARN_ON(ret))
        {
            dev_err(child->parent, "%s: sdio read from I/O failed (%d)\n", __func__, ret);
         goto io_err;
        }
        if(ret == 0)
#ifdef CONFIG_MMC_DISALLOW_STACK
            memcpy(buf, glue->brreg_data, reg_amount*4);
#else
            memcpy(buf, data, reg_amount*4);
#endif
        else
            memset(buf, 0xffffffff, reg_amount*4);
io_err:
        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
    }
    else
    {
        dev_err(child->parent, "sdio burst-read reg glue == NULL!!!\n");
    }
    return ret;
}
static int __must_check ssv6xxx_sdio_burst_write_reg(struct device *child, u32 *addr,
        u32 *buf, u8 reg_amount)
{
    int ret = (-1);
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func ;
#if !defined(CONFIG_MMC_DISALLOW_STACK) && defined(CONFIG_FW_ALIGNMENT_CHECK)
    PLATFORM_DMA_ALIGNED u8 data[MAX_BURST_WRITE_REG_AMOUNT][8]={{0},{0}};
#elif !defined(CONFIG_MMC_DISALLOW_STACK)
    u8 data[MAX_BURST_WRITE_REG_AMOUNT][8]={{0},{0}};
#endif
    u8 i = 0;
    if (IS_GLUE_INVALID(glue)){
        return ret;
    }

    if (reg_amount > MAX_BURST_WRITE_REG_AMOUNT)
    {
        HWIF_DBG_PRINT(glue->p_wlan_data, "The amount of sdio burst-read register must <= %d\n",
    MAX_BURST_WRITE_REG_AMOUNT);
        return ret;
    }
    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        for (i=0; i<reg_amount; i++)
        {
#ifdef CONFIG_MMC_DISALLOW_STACK
            glue->bwreg_data[i][0] = (addr[i] >> ( 0 )) &0xff;
            glue->bwreg_data[i][1] = (addr[i] >> ( 8 )) &0xff;
            glue->bwreg_data[i][2] = (addr[i] >> ( 16 )) &0xff;
            glue->bwreg_data[i][3] = (addr[i] >> ( 24 )) &0xff;
#else
            data[i][0] = (addr[i] >> ( 0 )) &0xff;
            data[i][1] = (addr[i] >> ( 8 )) &0xff;
            data[i][2] = (addr[i] >> ( 16 )) &0xff;
            data[i][3] = (addr[i] >> ( 24 )) &0xff;
#endif

            // 4 bytes data
#ifdef CONFIG_MMC_DISALLOW_STACK
            glue->bwreg_data[i][4] = (buf[i] >> ( 0 )) &0xff;
            glue->bwreg_data[i][5] = (buf[i] >> ( 8 )) &0xff;
            glue->bwreg_data[i][6] = (buf[i] >> ( 16 )) &0xff;
            glue->bwreg_data[i][7] = (buf[i] >> ( 24 )) &0xff;
#else
            data[i][4] = (buf[i] >> ( 0 )) &0xff;
            data[i][5] = (buf[i] >> ( 8 )) &0xff;
            data[i][6] = (buf[i] >> ( 16 )) &0xff;
            data[i][7] = (buf[i] >> ( 24 )) &0xff;            
#endif
        }        

#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_toio(func, IO_REG_BURST_WR_PORT_REG, glue->bwreg_data, reg_amount*8);
#else
        ret = sdio_memcpy_toio(func, IO_REG_BURST_WR_PORT_REG, data, reg_amount*8);
#endif
        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
    }
    else
    {
        dev_err(child->parent, "sdio burst-write reg glue == NULL!!!\n");
    }
    return ret;
}
static int __must_check ssv6xxx_sdio_burst_safe_write_reg(struct device *child, u32 *addr,
        u32 *buf, u8 reg_amount)
{
    int ret = (-1);
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func ;
#if !defined(CONFIG_MMC_DISALLOW_STACK) && defined(CONFIG_FW_ALIGNMENT_CHECK)
    PLATFORM_DMA_ALIGNED u8 data[MAX_BURST_WRITE_REG_AMOUNT][8]={{0},{0}};
#elif !defined(CONFIG_MMC_DISALLOW_STACK)
    u8 data[MAX_BURST_WRITE_REG_AMOUNT][8]={{0},{0}};
#endif
    u8 i = 0;
#if (defined(SSV_SUPPORT_SSV6006))
    int rdy_flag_cnt = 0;
#endif
    if (IS_GLUE_INVALID(glue)){
        return ret;
    }
    if (reg_amount > MAX_BURST_WRITE_REG_AMOUNT)
    {
        HWIF_DBG_PRINT(glue->p_wlan_data, "The amount of sdio burst-read register must <= %d\n",
    MAX_BURST_WRITE_REG_AMOUNT);
        return ret;
    }
    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        for (i=0; i<reg_amount; i++)
        {
            // 4 bytes address        
#ifdef CONFIG_MMC_DISALLOW_STACK
            glue->bwreg_data[i][0] = (addr[i] >> ( 0 )) &0xff;
            glue->bwreg_data[i][1] = (addr[i] >> ( 8 )) &0xff;
            glue->bwreg_data[i][2] = (addr[i] >> ( 16 )) &0xff;
            glue->bwreg_data[i][3] = (addr[i] >> ( 24 )) &0xff;
#else
            data[i][0] = (addr[i] >> ( 0 )) &0xff;
            data[i][1] = (addr[i] >> ( 8 )) &0xff;
            data[i][2] = (addr[i] >> ( 16 )) &0xff;
            data[i][3] = (addr[i] >> ( 24 )) &0xff;
#endif

            // 4 bytes data
#ifdef CONFIG_MMC_DISALLOW_STACK
            glue->bwreg_data[i][4] = (buf[i] >> ( 0 )) &0xff;
            glue->bwreg_data[i][5] = (buf[i] >> ( 8 )) &0xff;
            glue->bwreg_data[i][6] = (buf[i] >> ( 16 )) &0xff;
            glue->bwreg_data[i][7] = (buf[i] >> ( 24 )) &0xff;
#else
            data[i][4] = (buf[i] >> ( 0 )) &0xff;
            data[i][5] = (buf[i] >> ( 8 )) &0xff;
            data[i][6] = (buf[i] >> ( 16 )) &0xff;
            data[i][7] = (buf[i] >> ( 24 )) &0xff;            
#endif
        }        

#if (defined(SSV_SUPPORT_SSV6006))
        while(sdio_readb(func, REG_SD_READY_FLAG, &ret) != SDIO_READY_FLAG_IDLE) {
            if (ret != 0) {
                printk("%s: ret=%d", __func__, ret);
                goto io_err;
            } else if (++rdy_flag_cnt > SDIO_READY_FLAG_BUSY_THRESHOLD) {
                ret = -EBUSY;
                dev_err(&func->dev, "%s: bus is busy\n", __func__);
                goto io_err;
            }
            udelay(SDIO_READY_FLAG_BUSY_DELAY);
        }
#endif

#ifdef CONFIG_MMC_DISALLOW_STACK
        ret = sdio_memcpy_toio(func, IO_REG_BURST_WR_PORT_REG, glue->bwreg_data, reg_amount*8);
#else
        ret = sdio_memcpy_toio(func, IO_REG_BURST_WR_PORT_REG, data, reg_amount*8);
#endif
        if (WARN_ON(ret))
        {
            dev_err(child->parent, "%s: sdio write to I/O failed (%d)\n", __func__, ret);
            goto io_err;
        }
#if (defined(SSV_SUPPORT_SSV6006))
        while(sdio_readb(func, REG_SD_READY_FLAG, &ret) != SDIO_READY_FLAG_IDLE) {
            if (ret != 0) {
                printk("%s: ret=%d", __func__, ret);
                goto io_err;
            } else if (++rdy_flag_cnt > SDIO_READY_FLAG_BUSY_THRESHOLD) {
                ret = -EBUSY;
                dev_err(&func->dev, "%s: bus is busy\n", __func__);
                goto io_err;
            }
            udelay(SDIO_READY_FLAG_BUSY_DELAY);
        }
#endif
io_err:
        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
    }
    else
    {
        dev_err(child->parent, "sdio burst-write reg glue == NULL!!!\n");
    }
    return ret;
}
static int ssv6xxx_sdio_write_sram(struct device *child, u32 addr, u8 *data, u32 size)
{
    int ret = -1;
    struct ssv6xxx_sdio_glue *glue;
    struct sdio_func *func=NULL;
    glue = dev_get_drvdata(child->parent);
    if (IS_GLUE_INVALID(glue))
        return ret;
    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);
    do {
        if (ssv6xxx_sdio_write_reg(child,0xc0000860,addr)) ;
        sdio_writeb(func, 0x2, REG_Fn1_STATUS, &ret);
        if (unlikely(ret)) break;
        ret = sdio_memcpy_toio(func, glue->dataIOPort, data, size);
        if (unlikely(ret)) break;
        sdio_writeb(func, 0, REG_Fn1_STATUS, &ret);
        if (unlikely(ret)) break;
    } while (0);
    sdio_release_host(func);
    CHECK_IO_RET(glue, ret);
    return ret;
}
static int ssv6xxx_sdio_load_firmware(struct device *child, u32 start_addr, u8 *data, int data_length)
{
 return ssv6xxx_sdio_write_sram(child, start_addr, data, data_length);
}
static void ssv6xxx_sdio_load_fw_pre_config_hwif(struct device *child)
{
    struct ssv6xxx_sdio_glue *glue;
    struct sdio_func *func=NULL;
    glue = dev_get_drvdata(child->parent);
    if (!IS_GLUE_INVALID(glue)) {
        func = dev_to_sdio_func(glue->dev);
  ssv6xxx_low_sdio_clk(func);
 }
}
static void ssv6xxx_sdio_load_fw_post_config_hwif(struct device *child)
{
#ifndef SDIO_USE_SLOW_CLOCK
    struct ssv6xxx_sdio_glue *glue;
    struct sdio_func *func=NULL;
    glue = dev_get_drvdata(child->parent);
    if (!IS_GLUE_INVALID(glue)) {
        func = dev_to_sdio_func(glue->dev);
  ssv6xxx_high_sdio_clk(func);
 }
#endif
}
static int ssv6xxx_sdio_irq_getstatus(struct device *child,int *status)
{
    int ret = (-1);
    struct ssv6xxx_sdio_glue *glue;
    struct sdio_func *func;
    glue = dev_get_drvdata(child->parent);
    if (IS_GLUE_INVALID(glue))
  return ret;
    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        *status = sdio_readb(func, REG_INT_STATUS, &ret);
        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
    }
    return ret;
}
#if 0
static void _sdio_hexdump(const u8 *buf,
                             size_t len)
{
    size_t i;
    printk("\n-----------------------------\n");
    printk("hexdump(len=%lu):\n", (unsigned long) len);
    {
        for (i = 0; i < len; i++){
            printk(" %02x", buf[i]);
            if((i+1)%40 ==0)
                printk("\n");
        }
    }
    printk("\n-----------------------------\n");
}
#endif
static size_t ssv6xxx_sdio_get_readsz(struct device *child)
{
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func ;
    size_t size = 0;
    int ret = -1;
    u32 addr = SD_REG_BASE+REG_CARD_PKT_LEN_0;
    u32 buf;
    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);
    ret = ssv6xxx_sdio_safe_read_reg(child, addr, &buf);
    if (ret) {
        dev_err(child->parent, "sdio read len failed ret[%d]\n",ret);
        size = 0;
    } else {
        size = (size_t)(buf&0xffff);
    }
    sdio_release_host(func);
    return size;
}
static size_t ssv6xxx_sdio_get_aggr_readsz(struct device *child, int mode)
{
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func ;
#if !defined(CONFIG_MMC_DISALLOW_STACK) && defined(CONFIG_FW_ALIGNMENT_CHECK)
    PLATFORM_DMA_ALIGNED u32 size = 0;
#else
    u32 size = 0;
#endif
    int ret = -1;
    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);

#ifdef CONFIG_MMC_DISALLOW_STACK
    ret = sdio_memcpy_fromio(func, &glue->aggr_readsz, glue->dataIOPort, sizeof(u32)/* jmp_mpdu_len + accu_rx_len, total 4 bytes */);
#else
    ret = sdio_memcpy_fromio(func, &size, glue->dataIOPort, sizeof(u32)/* jmp_mpdu_len + accu_rx_len, total 4 bytes */);
#endif
    if (ret) { 
        dev_err(child->parent, "%s(): sdio read failed size ret[%d]\n", __func__, ret);
#ifdef CONFIG_MMC_DISALLOW_STACK
        glue->aggr_readsz = 0;
#else
        size = 0;
#endif
    }

#ifdef CONFIG_MMC_DISALLOW_STACK
    size = sdio_align_size(func, (glue->aggr_readsz >> 16));// accu_rx_len
#else
    size = sdio_align_size(func, (size >> 16));// accu_rx_len
#endif

    sdio_release_host(func);
    return (size_t)size;
}
static int __must_check ssv6xxx_sdio_read(struct device *child,
        void *buf, size_t *size, int mode)
{
    int ret = (-1), readsize = 0;
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func ;
    int readtype = 0;
    if (IS_GLUE_INVALID(glue))
  return ret;
    if (mode == RX_NORMAL_MODE) {
        *size = ssv6xxx_sdio_get_readsz(child);
        if (*size == 0)
            return ret;
    } else if ((mode == RX_HW_AGG_MODE) || (mode == RX_HW_AGG_MODE_METH3)) {
        if (*size == 0) {
            *size = ssv6xxx_sdio_get_aggr_readsz(child, mode);
            if (*size == 0)
                return ret;
        }
    } else {
        readtype = glue->p_wlan_data->rx_burstread_size(glue->p_wlan_data->rx_burstread_param);
        if (readtype == RX_BURSTREAD_SZ_FROM_CMD) {
            *size = ssv6xxx_sdio_get_readsz(child);
            if (*size == 0)
                return ret;
        } else if (readtype == RX_BURSTREAD_SZ_MAX_FRAME) {
            *size = MAX_FRAME_SIZE;
        } else {
            *size = MAX_FRAME_SIZE_DMG;
        }
    }
    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);
    readsize = sdio_align_size(func,*size);
    *size = readsize;
 ret = sdio_memcpy_fromio(func, buf, glue->dataIOPort, readsize);
    if (ret)
        dev_err(child->parent, "sdio read failed size ret[%d]\n",ret);
    sdio_release_host(func);
    CHECK_IO_RET(glue, ret);
#if 0
    if(*size > 1500)
        _sdio_hexdump(buf,*size);
#endif
    return ret;
}
static int __must_check ssv6xxx_sdio_write(struct device *child,
        void *buf, size_t len,u8 queue_num)
{
    int ret = (-1);
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
    int writesize;
    void *tempPointer;
 struct sk_buff *skb = (struct sk_buff *)buf;
    if (IS_GLUE_INVALID(glue))
  return ret;
    if ( glue != NULL )
    {
#ifdef CONFIG_FW_ALIGNMENT_CHECK
#ifdef CONFIG_ARM64
        if (((u64)(skb->data)) & 3) {
#else
        if (((u32)(skb->data)) & 3) {
#endif
            memcpy(glue->dmaSkb->data,skb->data,len);
            tempPointer = glue->dmaSkb->data;
        }
        else
#endif
            tempPointer = skb->data;
#if 0
        if(len > 1500)
            _sdio_hexdump(skb->data,len);
#endif
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        writesize = sdio_align_size(func,len);
        do
        {
            ret = sdio_memcpy_toio(func, glue->dataIOPort, tempPointer, writesize);
            if ( ret == -EILSEQ || ret == -ETIMEDOUT )
            {
                ret = -1;
                break;
            }
            else
            {
                if(ret)
                    dev_err(&func->dev,"Unexpected return value ret=[%d]\n",ret);
            }
        }
        while( ret == -EILSEQ || ret == -ETIMEDOUT);
        sdio_release_host(func);
        CHECK_IO_RET(glue, ret);
        if (ret)
            dev_err(&func->dev, "sdio write failed (%d)\n", ret);
    }
    return ret;
}
static void ssv6xxx_sdio_irq_handler(struct sdio_func *func)
{
    int status;
    struct ssv6xxx_sdio_glue *glue = sdio_get_drvdata(func);
    struct ssv6xxx_platform_data *pwlan_data;
    if (IS_GLUE_INVALID(glue))
        return;
    pwlan_data = glue->p_wlan_data;
    if (glue != NULL && glue->irq_handler != NULL)
    {
        atomic_set(&pwlan_data->irq_handling, 1);
        sdio_release_host(func);
        if ( glue->irq_handler != NULL )
            status = glue->irq_handler(0, glue->irq_dev);
        sdio_claim_host(func);
        atomic_set(&pwlan_data->irq_handling, 0);
    }
}
static void ssv6xxx_sdio_irq_setmask(struct device *child,int mask)
{
    int err_ret;
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
    if (IS_GLUE_INVALID(glue))
  return;
    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        sdio_writeb(func,mask, REG_INT_MASK, &err_ret);
        sdio_release_host(func);
        CHECK_IO_RET(glue, err_ret);
    }
}
static void ssv6xxx_sdio_irq_trigger(struct device *child)
{
    int err_ret;
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
    if (IS_GLUE_INVALID(glue))
  return;
    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        sdio_writeb(func,0x2, REG_INT_TRIGGER, &err_ret);
        sdio_release_host(func);
        CHECK_IO_RET(glue, err_ret);
    }
}
static int ssv6xxx_sdio_irq_getmask(struct device *child, u32 *mask)
{
    u8 imask = 0;
    int ret = (-1);
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
    if (IS_GLUE_INVALID(glue))
  return ret;
    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        imask = sdio_readb(func,REG_INT_MASK, &ret);
        *mask = imask;
        sdio_release_host(func);
    }
    return ret;
}
static void ssv6xxx_sdio_irq_enable(struct device *child)
{
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
    int ret;
    struct ssv6xxx_platform_data *pwlan_data;
    if (IS_GLUE_INVALID(glue))
        return;
    pwlan_data = glue->p_wlan_data;
    func = dev_to_sdio_func(glue->dev);
    sdio_claim_host(func);
    ret = sdio_claim_irq(func, ssv6xxx_sdio_irq_handler);
    if (ret)
        dev_err(&func->dev, "Failed to claim sdio irq: %d\n", ret);
    sdio_release_host(func);
    CHECK_IO_RET(glue, ret);
}
static void ssv6xxx_sdio_irq_disable(struct device *child, bool iswaitirq)
{
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
    struct ssv6xxx_platform_data *pwlan_data;
    int ret;
    if (IS_GLUE_INVALID(glue))
  return;
    HWIF_DBG_PRINT(glue->p_wlan_data, "ssv6xxx_sdio_irq_disable\n");
 pwlan_data = glue->p_wlan_data;
    func = dev_to_sdio_func(glue->dev);
    if (func == NULL) {
        HWIF_DBG_PRINT(glue->p_wlan_data, "func == NULL\n");
        return;
    }
    sdio_claim_host(func);
    while (atomic_read(&pwlan_data->irq_handling)) {
        sdio_release_host(func);
        schedule_timeout(HZ / 10);
        sdio_claim_host(func);
    }
    ret = sdio_release_irq(func);
    if (ret)
        dev_err(&func->dev, "Failed to release sdio irq: %d\n", ret);
    sdio_release_host(func);
}
static void ssv6xxx_sdio_irq_request(struct device *child,irq_handler_t irq_handler,void *irq_dev)
{
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
    bool isIrqEn = false;
    if (IS_GLUE_INVALID(glue))
  return;
    func = dev_to_sdio_func(glue->dev);
    glue->irq_handler = irq_handler;
    glue->irq_dev = irq_dev;
    if (isIrqEn)
    {
        ssv6xxx_sdio_irq_enable(child);
    }
}
static void ssv6xxx_sdio_read_parameter(struct sdio_func *func,
        struct ssv6xxx_sdio_glue *glue)
{
    int err_ret;
    sdio_claim_host(func);
    glue->dataIOPort = 0;
    glue->dataIOPort = glue->dataIOPort | (sdio_readb(func, REG_DATA_IO_PORT_0, &err_ret) << ( 8*0 ));
    glue->dataIOPort = glue->dataIOPort | (sdio_readb(func, REG_DATA_IO_PORT_1, &err_ret) << ( 8*1 ));
    glue->dataIOPort = glue->dataIOPort | (sdio_readb(func, REG_DATA_IO_PORT_2, &err_ret) << ( 8*2 ));
    glue->regIOPort = 0;
    glue->regIOPort = glue->regIOPort | (sdio_readb(func, REG_REG_IO_PORT_0, &err_ret) << ( 8*0 ));
    glue->regIOPort = glue->regIOPort | (sdio_readb(func, REG_REG_IO_PORT_1, &err_ret) << ( 8*1 ));
    glue->regIOPort = glue->regIOPort | (sdio_readb(func, REG_REG_IO_PORT_2, &err_ret) << ( 8*2 ));
    dev_err(&func->dev, "dataIOPort 0x%x regIOPort 0x%x\n",glue->dataIOPort,glue->regIOPort);
#ifdef CONFIG_PLATFORM_SDIO_BLOCK_SIZE
    err_ret = sdio_set_block_size(func,CONFIG_PLATFORM_SDIO_BLOCK_SIZE);
#else
    err_ret = sdio_set_block_size(func,SDIO_DEF_BLOCK_SIZE);
#endif
    if (err_ret != 0) {
        printk("SDIO setting SDIO_DEF_BLOCK_SIZE fail!!\n");
    }
#ifdef CONFIG_PLATFORM_SDIO_OUTPUT_TIMING
    sdio_writeb(func, CONFIG_PLATFORM_SDIO_OUTPUT_TIMING,REG_OUTPUT_TIMING_REG, &err_ret);
#else
    sdio_writeb(func, SDIO_DEF_OUTPUT_TIMING,REG_OUTPUT_TIMING_REG, &err_ret);
#endif
    sdio_writeb(func, 0x00,REG_Fn1_STATUS, &err_ret);
#if 0
    sdio_writeb(func,SDIO_TX_ALLOC_SIZE_SHIFT|SDIO_TX_ALLOC_ENABLE,REG_SDIO_TX_ALLOC_SHIFT, &err_ret);
#endif
    sdio_release_host(func);
}
static void ssv6xxx_do_sdio_wakeup(struct sdio_func *func)
{
 int err_ret;
 if(func != NULL)
 {
  sdio_claim_host(func);
  sdio_writeb(func, 0x01, REG_PMU_WAKEUP, &err_ret);
  mdelay(10);
  sdio_writeb(func, 0x00, REG_PMU_WAKEUP, &err_ret);
  sdio_release_host(func);
 }
}
static void ssv6xxx_sdio_pmu_wakeup(struct device *child)
{
 struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
 struct sdio_func *func;
 if (glue != NULL) {
  func = dev_to_sdio_func(glue->dev);
  ssv6xxx_do_sdio_wakeup(func);
 }
}
static bool ssv6xxx_sdio_support_scatter(struct device *child)
{
    bool support = false;
    #if LINUX_VERSION_CODE > KERNEL_VERSION(3,0,0)
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
    do {
        if (IS_GLUE_INVALID(glue)) {
            dev_err(child, "ssv6xxx_sdio_enable_scatter glue == NULL!!!\n");
            break;
        }
        func = dev_to_sdio_func(glue->dev);
        if (func->card->host->max_segs < MAX_SCATTER_ENTRIES_PER_REQ) {
            dev_err(&func->dev, "host controller only supports scatter of :%d entries, driver need: %d\n",
            func->card->host->max_segs,
            MAX_SCATTER_ENTRIES_PER_REQ);
            break;
     }
        support = true;
    } while (0);
    #endif
    return support;
}
static void ssv6xxx_sdio_setup_scat_data(struct sdio_scatter_req *scat_req,
     struct mmc_data *data)
{
 struct scatterlist *sg;
 int i;
 data->blksz = SDIO_DEF_BLOCK_SIZE;
 data->blocks = scat_req->len / SDIO_DEF_BLOCK_SIZE;
 printk("scatter: (%s)  (block len: %d, block count: %d) , (tot:%d,sg:%d)\n",
     (scat_req->req & SDIO_WRITE) ? "WR" : "RD",
     data->blksz, data->blocks, scat_req->len,
     scat_req->scat_entries);
 data->flags = (scat_req->req & SDIO_WRITE) ? MMC_DATA_WRITE :
          MMC_DATA_READ;
 sg = scat_req->sgentries;
 sg_init_table(sg, scat_req->scat_entries);
 for (i = 0; i < scat_req->scat_entries; i++, sg++) {
  printk("%d: addr:0x%p, len:%d\n",
      i, scat_req->scat_list[i].buf,
      scat_req->scat_list[i].len);
  sg_set_buf(sg, scat_req->scat_list[i].buf,
      scat_req->scat_list[i].len);
 }
 data->sg = scat_req->sgentries;
 data->sg_len = scat_req->scat_entries;
}
static inline void ssv6xxx_sdio_set_cmd53_arg(u32 *arg, u8 rw, u8 func,
          u8 mode, u8 opcode, u32 addr,
          u16 blksz)
{
 *arg = (((rw & 1) << 31) |
  ((func & 0x7) << 28) |
  ((mode & 1) << 27) |
  ((opcode & 1) << 26) |
  ((addr & 0x1FFFF) << 9) |
  (blksz & 0x1FF));
}
static int ssv6xxx_sdio_rw_scatter(struct device *child,
          struct sdio_scatter_req *scat_req)
{
    struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
    struct sdio_func *func;
 struct mmc_request mmc_req;
 struct mmc_command cmd;
 struct mmc_data data;
 u8 opcode, rw;
 int status = 1;
    do{
        if(!glue){
            dev_err(child, "ssv6xxx_sdio_enable_scatter glue == NULL!!!\n");
            break;
        }
        func = dev_to_sdio_func(glue->dev);
     memset(&mmc_req, 0, sizeof(struct mmc_request));
     memset(&cmd, 0, sizeof(struct mmc_command));
     memset(&data, 0, sizeof(struct mmc_data));
     ssv6xxx_sdio_setup_scat_data(scat_req, &data);
     opcode = 0;
     rw = (scat_req->req & SDIO_WRITE) ? CMD53_ARG_WRITE : CMD53_ARG_READ;
     ssv6xxx_sdio_set_cmd53_arg(&cmd.arg, rw, func->num,
          CMD53_ARG_BLOCK_BASIS, opcode, glue->dataIOPort,
          data.blocks);
     cmd.opcode = SD_IO_RW_EXTENDED;
     cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;
     mmc_req.cmd = &cmd;
     mmc_req.data = &data;
     mmc_set_data_timeout(&data, func->card);
     mmc_wait_for_req(func->card->host, &mmc_req);
     status = cmd.error ? cmd.error : data.error;
        if (cmd.error)
      return cmd.error;
     if (data.error)
      return data.error;
    }while(0);
 return status;
}
static void ssv6xxx_set_sdio_clk(struct sdio_func *func, u32 sdio_hz)
{
 struct mmc_host *host;
 host = func->card->host;
 if (sdio_hz < host->f_min)
  sdio_hz = host->f_min;
 else if (sdio_hz > host->f_max)
  sdio_hz = host->f_max;
 printk("%s: set sdio clk %dHz\n", __FUNCTION__, sdio_hz);
 sdio_claim_host(func);
 host->ios.clock = sdio_hz;
 host->ops->set_ios(host, &host->ios);
 mdelay(20);
 sdio_release_host(func);
}
static void ssv6xxx_low_sdio_clk(struct sdio_func *func)
{
 ssv6xxx_set_sdio_clk(func, LOW_SPEED_SDIO_CLOCK);
}
static void ssv6xxx_high_sdio_clk(struct sdio_func *func)
{
#ifndef SDIO_USE_SLOW_CLOCK
 ssv6xxx_set_sdio_clk(func, HIGH_SPEED_SDIO_CLOCK);
#endif
}
static void ssv6xxx_sdio_reset(struct device *child)
{
 struct ssv6xxx_sdio_glue *glue = dev_get_drvdata(child->parent);
 struct sdio_func *func = dev_to_sdio_func(glue->dev);
    if (IS_GLUE_INVALID(glue))
  return;
 HWIF_DBG_PRINT(glue->p_wlan_data, "%s\n", __FUNCTION__);
    ssv6xxx_do_sdio_reset_reinit(glue->p_wlan_data, func, glue);
}
static int ssv6xxx_sdio_property(struct device *child)
{
 return SSV_HWIF_CAPABILITY_INTERRUPT | SSV_HWIF_INTERFACE_SDIO;
}
static void ssv6xxx_sdio_sysplf_reset(struct device *child, u32 addr, u32 value)
{
    int retval = 0;
    retval = ssv6xxx_sdio_write_reg(child, addr, value);
    if (retval)
        printk("Fail to reset sysplf.\n");
}
static struct ssv6xxx_hwif_ops sdio_ops =
{
    .read = ssv6xxx_sdio_read,
    .write = ssv6xxx_sdio_write,
    .readreg = ssv6xxx_sdio_read_reg,
    .writereg = ssv6xxx_sdio_write_reg,
    .safe_readreg = ssv6xxx_sdio_safe_read_reg,
    .safe_writereg = ssv6xxx_sdio_safe_write_reg,
    .burst_readreg = ssv6xxx_sdio_burst_read_reg,
    .burst_writereg = ssv6xxx_sdio_burst_write_reg,
    .burst_safe_readreg = ssv6xxx_sdio_burst_safe_read_reg,
    .burst_safe_writereg = ssv6xxx_sdio_burst_safe_write_reg,
#ifdef ENABLE_WAKE_IO_ISR_WHEN_HCI_ENQUEUE
    .trigger_tx_rx = ssv6xxx_sdio_trigger_tx_rx,
#endif
    .irq_getmask = ssv6xxx_sdio_irq_getmask,
    .irq_setmask = ssv6xxx_sdio_irq_setmask,
    .irq_enable = ssv6xxx_sdio_irq_enable,
    .irq_disable = ssv6xxx_sdio_irq_disable,
    .irq_getstatus = ssv6xxx_sdio_irq_getstatus,
    .irq_request = ssv6xxx_sdio_irq_request,
    .irq_trigger = ssv6xxx_sdio_irq_trigger,
    .pmu_wakeup = ssv6xxx_sdio_pmu_wakeup,
    .load_fw = ssv6xxx_sdio_load_firmware,
    .load_fw_pre_config_device = ssv6xxx_sdio_load_fw_pre_config_hwif,
    .load_fw_post_config_device = ssv6xxx_sdio_load_fw_post_config_hwif,
    .cmd52_read = ssv6xxx_sdio_cmd52_read,
    .cmd52_write = ssv6xxx_sdio_cmd52_write,
    .support_scatter = ssv6xxx_sdio_support_scatter,
    .rw_scatter = ssv6xxx_sdio_rw_scatter,
    .is_ready = ssv6xxx_is_ready,
    .write_sram = ssv6xxx_sdio_write_sram,
 .interface_reset = ssv6xxx_sdio_reset,
    .property = ssv6xxx_sdio_property,
 .sysplf_reset = ssv6xxx_sdio_sysplf_reset,
};
#ifdef CONFIG_PCIEASPM
#include <linux/pci.h>
#include <linux/pci-aspm.h>
static int cabrio_sdio_pm_check(struct sdio_func *func)
{
 struct pci_dev *pci_dev = NULL;
 struct mmc_card *card = func->card;
 struct mmc_host *host = card->host;
 if (strcmp(host->parent->bus->name, "pci"))
 {
  dev_info(&func->dev, "SDIO host is not PCI device, but \"%s\".", host->parent->bus->name);
  return 0;
 }
 for_each_pci_dev(pci_dev) {
  if ( ((pci_dev->class >> 8) != PCI_CLASS_SYSTEM_SDHCI)
   && ( (pci_dev->driver == NULL)
    || (strcmp(pci_dev->driver->name, "sdhci-pci") != 0)))
   continue;
  if (pci_is_pcie(pci_dev)) {
   u8 aspm;
   int pos;
   pos = pci_pcie_cap(pci_dev);
         if (pos) {
             struct pci_dev *parent = pci_dev->bus->self;
             pci_read_config_byte(pci_dev, pos + PCI_EXP_LNKCTL, &aspm);
             aspm &= ~(PCIE_LINK_STATE_L0S | PCIE_LINK_STATE_L1);
             pci_write_config_byte(pci_dev, pos + PCI_EXP_LNKCTL, aspm);
             pos = pci_pcie_cap(parent);
             pci_read_config_byte(parent, pos + PCI_EXP_LNKCTL, &aspm);
             aspm &= ~(PCIE_LINK_STATE_L0S | PCIE_LINK_STATE_L1);
             pci_write_config_byte(parent, pos + PCI_EXP_LNKCTL, aspm);
             dev_info(&pci_dev->dev, "Clear PCI-E device and its parent link state L0S and L1 and CLKPM.\n");
         }
  }
 }
 return 0;
}
#endif
static int ssv6xxx_sdio_power_on(struct ssv6xxx_platform_data * pdata, struct sdio_func *func)
{
 int ret = 0;
 if (pdata->is_enabled == true)
  return 0;
    printk("ssv6xxx_sdio_power_on\n");
 sdio_claim_host(func);
 ret = sdio_enable_func(func);
 sdio_release_host(func);
 if (ret) {
  printk("Unable to enable sdio func: %d)\n", ret);
  return ret;
 }
 msleep(10);
 pdata->is_enabled = true;
 return ret;
}
static int ssv6xxx_do_sdio_init_seq_5537(struct sdio_func *func) {
    int status = 1;
    struct mmc_command cmd = {0};
    cmd.opcode = SD_IO_SEND_OP_COND;
    cmd.arg = 0;
    cmd.flags = MMC_RSP_SPI_R4 | MMC_RSP_R4 | MMC_CMD_BCR;
    sdio_claim_host(func);
    status = mmc_wait_for_cmd(func->card->host, &cmd, 0);
    sdio_release_host(func);
    if (status != 0) {
        printk("%s(): The 1st CMD5 failed.", __func__);
        return -1;
    }
    cmd.opcode = SD_IO_SEND_OP_COND;
    cmd.arg = MMC_VDD_30_31|MMC_VDD_31_32|MMC_VDD_32_33|MMC_VDD_33_34|MMC_VDD_34_35;
    cmd.flags = MMC_RSP_SPI_R4 | MMC_RSP_R4 | MMC_CMD_BCR;
    sdio_claim_host(func);
    status = mmc_wait_for_cmd(func->card->host, &cmd, 0);
    sdio_release_host(func);
    if (status != 0) {
        printk("%s(): The 2nd CMD5 failed.", __func__);
        return -1;
    }
    cmd.opcode = SD_SEND_RELATIVE_ADDR;
    cmd.arg = 0;
    cmd.flags = MMC_RSP_R6 | MMC_CMD_BCR;
    sdio_claim_host(func);
    status = mmc_wait_for_cmd(func->card->host, &cmd, 0);
    sdio_release_host(func);
    if (status == 0) {
        func->card->rca = cmd.resp[0] >> 16;
    } else {
        printk("%s(): CMD3 failed.", __func__);
        return -1;
    }
    cmd.opcode = MMC_SELECT_CARD;
    cmd.arg = func->card->rca << 16;
    cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
    sdio_claim_host(func);
    status = mmc_wait_for_cmd(func->card->host, &cmd, 0);
    sdio_release_host(func);
    if (status != 0) {
        printk("%s(): CMD7 failed.", __func__);
        return -1;
    }
    return 0;
}
static void ssv6xxx_do_sdio_reset_reinit(struct ssv6xxx_platform_data *pwlan_data,
        struct sdio_func *func, struct ssv6xxx_sdio_glue *glue)
{
    int err_ret;
    struct mmc_host *host;
    if (IS_GLUE_INVALID(glue)) {
        printk("%s(): glue is invalid.\n", __func__);
        return;
    }
    sdio_claim_host(func);
    sdio_f0_writeb(func, 0x08, SDIO_CCCR_ABORT, &err_ret);
    sdio_release_host(func);
    CHECK_IO_RET(glue, err_ret);
    err_ret = ssv6xxx_do_sdio_init_seq_5537(func);
    CHECK_IO_RET(glue, err_ret);
    sdio_claim_host(func);
    host = func->card->host;
    host->ios.bus_width = MMC_BUS_WIDTH_4;
    host->ops->set_ios(host, &host->ios);
    mdelay(20);
    sdio_release_host(func);
    sdio_claim_host(func);
    sdio_f0_writeb(func, SDIO_BUS_WIDTH_4BIT, SDIO_CCCR_IF, &err_ret);
    sdio_release_host(func);
    CHECK_IO_RET(glue, err_ret);
    ssv6xxx_sdio_power_on(pwlan_data, func);
    ssv6xxx_sdio_read_parameter(func, glue);
}
static void ssv6xxx_sdio_direct_int_mux_mode(struct ssv6xxx_sdio_glue *glue, bool enable)
{
    int err_ret = (-1);
    struct sdio_func *func;
 u8 host_cfg;
    if (IS_GLUE_INVALID(glue))
  return;
    if (glue != NULL)
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);
        host_cfg = sdio_readb(func, MCU_NOTIFY_HOST_CFG, &err_ret);
  if (err_ret == 0) {
   if (!enable) {
    host_cfg &= ~(0x04);
          sdio_writeb(func, host_cfg, MCU_NOTIFY_HOST_CFG, &err_ret);
   } else {
    host_cfg |= (0x04);
          sdio_writeb(func, host_cfg, MCU_NOTIFY_HOST_CFG, &err_ret);
   }
  }
        sdio_release_host(func);
        CHECK_IO_RET(glue, err_ret);
    }
}
static int ssv6xxx_sdio_power_off(struct ssv6xxx_platform_data * pdata, struct sdio_func *func)
{
 int ret;
 if (pdata->is_enabled == false)
  return 0;
    printk("ssv6xxx_sdio_power_off\n");
 sdio_claim_host(func);
 ret = sdio_disable_func(func);
 sdio_release_host(func);
 if (ret)
  return ret;
 pdata->is_enabled = false;
 return ret;
}
static void _read_chip_id (struct ssv6xxx_sdio_glue *glue)
{
    u32 regval;
    int ret;
    u8 _chip_id[SSV6XXX_CHIP_ID_LENGTH];
    u8 *c = _chip_id;
    int i = 0;
    ret = __ssv6xxx_sdio_read_reg(glue, ADR_CHIP_ID_3, &regval);
    *((u32 *)&_chip_id[0]) = __be32_to_cpu(regval);
    if (ret == 0)
        ret = __ssv6xxx_sdio_read_reg(glue, ADR_CHIP_ID_2, &regval);
    *((u32 *)&_chip_id[4]) = __be32_to_cpu(regval);
    if (ret == 0)
        ret = __ssv6xxx_sdio_read_reg(glue, ADR_CHIP_ID_1, &regval);
    *((u32 *)&_chip_id[8]) = __be32_to_cpu(regval);
    if (ret == 0)
        ret = __ssv6xxx_sdio_read_reg(glue, ADR_CHIP_ID_0, &regval);
    *((u32 *)&_chip_id[12]) = __be32_to_cpu(regval);
    _chip_id[12+sizeof(u32)] = 0;
    while (*c == 0) {
        i++;
        c++;
        if (i == 16) {
            c = _chip_id;
            break;
        }
    }
    if (*c != 0) {
        strncpy(glue->tmp_data.chip_id, c, SSV6XXX_CHIP_ID_LENGTH);
        dev_info(glue->dev, "CHIP ID: %s \n", glue->tmp_data.chip_id);
        strncpy(glue->tmp_data.short_chip_id, c, SSV6XXX_CHIP_ID_SHORT_LENGTH);
        glue->tmp_data.short_chip_id[SSV6XXX_CHIP_ID_SHORT_LENGTH] = 0;
    } else {
        dev_err(glue->dev, "Failed to read chip ID");
        glue->tmp_data.chip_id[0] = 0;
    }
    if ( strstr(glue->tmp_data.chip_id, SSV6051_CHIP)
        || strstr(glue->tmp_data.chip_id, SSV6051_CHIP_ECO3)) {
        struct ssv6xxx_platform_data *pwlan_data;
        pwlan_data = &glue->tmp_data;
        pwlan_data->ops->safe_readreg = ssv6xxx_sdio_read_reg;
        pwlan_data->ops->safe_writereg = ssv6xxx_sdio_write_reg;
        printk("SWAP ops for 6051\n");
    }
}
#if (defined(CONFIG_SSV_SDIO_INPUT_DELAY) && defined(CONFIG_SSV_SDIO_OUTPUT_DELAY))
static void ssv6xxx_sdio_delay_chain(struct sdio_func *func, u32 input_delay, u32 output_delay)
{
    u8 in_delay, out_delay;
    u8 delay[4];
    int ret = 0, i = 0;
    if ((input_delay == 0) && (output_delay == 0))
        return;
    for (i = 0; i < 4; i++) {
        delay[i] = 0;
        in_delay = (input_delay >> ( i * 8 )) & 0xff;
        out_delay = (output_delay >> ( i * 8 )) & 0xff;
        if (in_delay == SDIO_DELAY_LEVEL_OFF)
            delay[i] |= (1 << SDIO_INPUT_DELAY_SFT);
        else
            delay[i] |= ((in_delay-1) << SDIO_INPUT_DELAY_LEVEL_SFT);
        if (out_delay == SDIO_DELAY_LEVEL_OFF)
            delay[i] |= (1 << SDIO_OUTPUT_DELAY_SFT);
        else
            delay[i] |= ((out_delay-1) << SDIO_OUTPUT_DELAY_LEVEL_SFT);
    }
    printk("%s: delay chain data0[%02x], data1[%02x], data2[%02x], data3[%02x]\n",
        __FUNCTION__, delay[0], delay[1], delay[2], delay[3]);
    sdio_claim_host(func);
    sdio_writeb(func, delay[0], REG_SDIO_DAT0_DELAY, &ret);
    sdio_writeb(func, delay[1], REG_SDIO_DAT1_DELAY, &ret);
    sdio_writeb(func, delay[2], REG_SDIO_DAT2_DELAY, &ret);
    sdio_writeb(func, delay[3], REG_SDIO_DAT3_DELAY, &ret);
 sdio_release_host(func);
}
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
int __devinit tu_ssv6xxx_sdio_probe(struct sdio_func *func,
        const struct sdio_device_id *id)
#else
int tu_ssv6xxx_sdio_probe(struct sdio_func *func,
        const struct sdio_device_id *id)
#endif
{
    struct ssv6xxx_platform_data *pwlan_data;
    struct ssv6xxx_sdio_glue *glue;
    int ret = -ENOMEM;
    printk(KERN_INFO "=======================================\n");
    printk(KERN_INFO "==           RUN SDIO                ==\n");
    printk(KERN_INFO "=======================================\n");
    if (func->num != 0x01)
        return -ENODEV;
    glue = kzalloc(sizeof(*glue), GFP_KERNEL);
    if (!glue)
    {
        dev_err(&func->dev, "can't allocate glue\n");
        goto out;
    }
#if (defined(CONFIG_SSV_SDIO_INPUT_DELAY) && defined(CONFIG_SSV_SDIO_OUTPUT_DELAY))
    ssv6xxx_sdio_delay_chain(func, CONFIG_SSV_SDIO_INPUT_DELAY, CONFIG_SSV_SDIO_OUTPUT_DELAY);
#endif
 ssv6xxx_low_sdio_clk(func);
#ifdef CONFIG_FW_ALIGNMENT_CHECK
    glue->dmaSkb=__dev_alloc_skb(SDIO_DMA_BUFFER_LEN , GFP_KERNEL);
#endif
    pwlan_data = &glue->tmp_data;
    memset(pwlan_data, 0, sizeof(struct ssv6xxx_platform_data));
    atomic_set(&pwlan_data->irq_handling, 0);
    glue->dev = &func->dev;
    func->card->quirks |= MMC_QUIRK_LENIENT_FN0;
    func->card->quirks |= MMC_QUIRK_BLKSZ_FOR_BYTE_MODE;
    glue->dev_ready = true;
    pwlan_data->vendor = func->vendor;
    pwlan_data->device = func->device;
    dev_err(glue->dev, "vendor = 0x%x device = 0x%x\n",
            pwlan_data->vendor, pwlan_data->device);
    #ifdef CONFIG_PCIEASPM
    cabrio_sdio_pm_check(func);
    #endif
    pwlan_data->ops = &sdio_ops;
    sdio_set_drvdata(func, glue);
#ifdef CONFIG_PM
#if 0
    ssv6xxx_do_sdio_wakeup(func);
#endif
#endif
    ssv6xxx_sdio_power_on(pwlan_data, func);
    ssv6xxx_sdio_read_parameter(func, glue);
    ssv6xxx_do_sdio_reset_reinit(pwlan_data, func, glue);
 ssv6xxx_sdio_direct_int_mux_mode(glue, false);
    _read_chip_id(glue);
    glue->core = platform_device_alloc(pwlan_data->short_chip_id, -1);
    if (!glue->core)
    {
        dev_err(glue->dev, "can't allocate platform_device");
        ret = -ENOMEM;
        goto out_free_glue;
    }
    glue->core->dev.parent = &func->dev;
    ret = platform_device_add_data(glue->core, pwlan_data,
                                   sizeof(*pwlan_data));
    if (ret)
    {
        dev_err(glue->dev, "can't add platform data\n");
        goto out_dev_put;
    }
    glue->p_wlan_data = glue->core->dev.platform_data;
    ret = platform_device_add(glue->core);
    if (ret)
    {
        dev_err(glue->dev, "can't add platform device\n");
        goto out_dev_put;
    }
    ssv6xxx_sdio_irq_setmask(&glue->core->dev,0xff);
#if 0
    ssv6xxx_sdio_irq_enable(&glue->core->dev);
#else
#endif
#if 0
    glue->dev->platform_data = (void *)pwlan_data;
    ret = tu_ssv6xxx_dev_probe(glue->dev);
    if (ret)
    {
        dev_err(glue->dev, "failed to initial ssv6xxx device !!\n");
        platform_device_del(glue->core);
        goto out_dev_put;
    }
#endif
    return 0;
out_dev_put:
    platform_device_put(glue->core);
out_free_glue:
    if (glue != NULL)
        kfree(glue);
out:
    return ret;
}
EXPORT_SYMBOL(tu_ssv6xxx_sdio_probe);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
void __devexit tu_ssv6xxx_sdio_remove(struct sdio_func *func)
#else
void tu_ssv6xxx_sdio_remove(struct sdio_func *func)
#endif
{
    struct ssv6xxx_sdio_glue *glue = sdio_get_drvdata(func);
    printk("tu_ssv6xxx_sdio_remove..........\n");
    if ( glue )
    {
        printk("tu_ssv6xxx_sdio_remove - ssv6xxx_sdio_irq_disable\n");
        ssv6xxx_sdio_irq_disable(&glue->core->dev,false);
        glue->dev_ready = false;
#if 0
        tu_ssv6xxx_dev_remove(glue->dev);
#endif
  ssv6xxx_low_sdio_clk(func);
#ifdef CONFIG_FW_ALIGNMENT_CHECK
  if(glue->dmaSkb != NULL)
         dev_kfree_skb(glue->dmaSkb);
#endif
        printk("tu_ssv6xxx_sdio_remove - disable mask\n");
        ssv6xxx_sdio_irq_setmask(&glue->core->dev,0xff);
        ssv6xxx_sdio_power_off(glue->p_wlan_data, func);
        printk("platform_device_del \n");
        platform_device_del(glue->core);
        printk("platform_device_put \n");
        platform_device_put(glue->core);
        kfree(glue);
    }
    sdio_set_drvdata(func, NULL);
    printk("tu_ssv6xxx_sdio_remove leave..........\n");
}
EXPORT_SYMBOL(tu_ssv6xxx_sdio_remove);

#ifdef CONFIG_PM
int tu_ssv6xxx_sdio_suspend(struct device *dev)
{
    struct sdio_func *func = dev_to_sdio_func(dev);
    struct ssv6xxx_sdio_glue *glue = sdio_get_drvdata(func);
    mmc_pm_flag_t flags = sdio_get_host_pm_caps(func);
    int ret = 0;
    dev_info(dev, "%s: suspend: PM flags = 0x%x\n",
             sdio_func_id(func), flags);
 ssv6xxx_low_sdio_clk(func);
 glue->p_wlan_data->suspend(glue->p_wlan_data->pm_param);
    if (!(flags & MMC_PM_KEEP_POWER))
    {
     dev_err(dev, "%s: cannot remain alive while host is suspended\n",
                sdio_func_id(func));
    }
    ret = sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
    if (ret)
     return ret;
    #if 0
    if (softc->wow_enabled)
    {
        sdio_flags = sdio_get_host_pm_caps(func);
        if (!(sdio_flags & MMC_PM_KEEP_POWER))
        {
            dev_err(dev, "can't keep power while host "
                    "is suspended\n");
            ret = -EINVAL;
            goto out;
        }
        ret = sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
        if (ret)
        {
            dev_err(dev, "error while trying to keep power\n");
            goto out;
        }
    }else{
        ssv6xxx_sdio_irq_disable(&glue->core->dev,true);
    }
#endif
    return ret;
}
EXPORT_SYMBOL(tu_ssv6xxx_sdio_suspend);

int tu_ssv6xxx_sdio_resume(struct device *dev)
{
    struct sdio_func *func = dev_to_sdio_func(dev);
    struct ssv6xxx_sdio_glue *glue = sdio_get_drvdata(func);
    dev_info(dev, "%s: resume.\n", __FUNCTION__);
 if (!glue)
  return 0;
 glue->p_wlan_data->resume(glue->p_wlan_data->pm_param);
    return 0;
}
EXPORT_SYMBOL(tu_ssv6xxx_sdio_resume);

static const struct dev_pm_ops ssv6xxx_sdio_pm_ops =
{
    .suspend = tu_ssv6xxx_sdio_suspend,
    .resume = tu_ssv6xxx_sdio_resume,
};
#endif
struct sdio_driver tu_ssv6xxx_sdio_driver =
{
    .name = "TU_SSV6XXX_SDIO",
    .id_table = ssv6xxx_sdio_devices,
    .probe = tu_ssv6xxx_sdio_probe,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
    .remove = __devexit_p(tu_ssv6xxx_sdio_remove),
#else
    .remove = tu_ssv6xxx_sdio_remove,
#endif
#ifdef CONFIG_PM
    .drv = {
        .pm = &ssv6xxx_sdio_pm_ops,
    },
#endif
};
EXPORT_SYMBOL(tu_ssv6xxx_sdio_driver);
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
int tu_ssv6xxx_sdio_init(void)
#else
static int __init tu_ssv6xxx_sdio_init(void)
#endif
{
    printk(KERN_INFO "tu_ssv6xxx_sdio_init\n");
    return sdio_register_driver(&tu_ssv6xxx_sdio_driver);
}
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
void tu_ssv6xxx_sdio_exit(void)
#else
static void __exit tu_ssv6xxx_sdio_exit(void)
#endif
{
    printk(KERN_INFO "tu_ssv6xxx_sdio_exit\n");
    sdio_unregister_driver(&tu_ssv6xxx_sdio_driver);
}
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
EXPORT_SYMBOL(tu_ssv6xxx_sdio_init);
EXPORT_SYMBOL(tu_ssv6xxx_sdio_exit);
#else
module_init(tu_ssv6xxx_sdio_init);
module_exit(tu_ssv6xxx_sdio_exit);
#endif
MODULE_LICENSE("GPL");
