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

#include <linux/version.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
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

#include <hwif_ctrl/hwif.h>
#include "sdio_def.h"
#include <ssv_chip_id.h>

#include "sdio.h"
#define LOW_SPEED_SDIO_CLOCK		(25000000)
#define HIGH_SPEED_SDIO_CLOCK		(50000000)

#define MAX_RX_FRAME_SIZE 0x900
#define MAX_REG_RETRY_CNT	(3)

#define SSV_VENDOR_ID	0x3030
#define SSV_CABRIO_DEVID	0x3030

#define CHECK_IO_RET(GLUE, RET) \
    do { \
        if (RET) { \
            if ((++((GLUE)->err_count)) > MAX_ERR_COUNT) \
                printk(KERN_ERR "MAX SDIO Error\n"); \
        } else \
            (GLUE)->err_count = 0; \
    } while (0)
    
#define MAX_ERR_COUNT		(10)

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
extern int __devinit ssv6xxx_sdio_probe(struct sdio_func *func,
        const struct sdio_device_id *id);
#else
extern int ssv6xxx_sdio_probe(struct sdio_func *func,
        const struct sdio_device_id *id);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
extern int __devinit tu_ssv6xxx_sdio_probe(struct sdio_func *func,
        const struct sdio_device_id *id);
#else
extern int tu_ssv6xxx_sdio_probe(struct sdio_func *func,
        const struct sdio_device_id *id);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
extern void __devexit ssv6xxx_sdio_remove(struct sdio_func *func);
#else
extern void ssv6xxx_sdio_remove(struct sdio_func *func);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
extern void __devexit tu_ssv6xxx_sdio_remove(struct sdio_func *func);
#else
extern void tu_ssv6xxx_sdio_remove(struct sdio_func *func);
#endif

extern int ssv6xxx_sdio_suspend(struct device *dev);
extern int tu_ssv6xxx_sdio_suspend(struct device *dev);
extern int ssv6xxx_sdio_resume(struct device *dev);
extern int tu_ssv6xxx_sdio_resume(struct device *dev);

int sdio_product = -1;

#define CABRIO_SDIO_DEV    0x6051
#define TURISMO_SDIO_DEV   0x6006

struct hwif_ctrl_glue
{
    struct device                *dev;
    struct turismo_platform_data  tmp_data;
    //u8                          chip_id[SSV6XXX_CHIP_ID_LENGTH+1];
    //u8                          short_chip_id[SSV6XXX_CHIP_ID_SHORT_LENGTH+1];
    /* for turismo SDIO */
    unsigned int                dataIOPort;
    unsigned int                regIOPort;

    unsigned int                err_count;
};

#define IS_GLUE_INVALID(glue)  \
      (   (glue == NULL) \
       || (glue->err_count > MAX_ERR_COUNT))

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
static const struct sdio_device_id hwif_ctrl_devices[] __devinitconst =
#else
static const struct sdio_device_id hwif_ctrl_devices[] =
#endif
{
    { SDIO_DEVICE(SSV_VENDOR_ID, SSV_CABRIO_DEVID) },
    {}
};
MODULE_DEVICE_TABLE(sdio, hwif_ctrl_devices);

static int __must_check __hwif_ctrl_read_reg (struct hwif_ctrl_glue *glue, u32 addr,
                                                 u32 *buf)
{    
    int ret = (-1);
    struct sdio_func *func ;
#if 1//#ifdef CONFIG_AMLOGIC_MODIFY
    u8 *data = NULL;   
    data = kzalloc(sizeof(u8)*4, GFP_KERNEL);
    if (!data)
         return ret;
#else
#ifdef CONFIG_FW_ALIGNMENT_CHECK
    PLATFORM_DMA_ALIGNED u8 data[4];
#else
    u8 data[4];
#endif
#endif
        
    if (IS_GLUE_INVALID(glue)) {
#if 1//#ifdef CONFIG_AMLOGIC_MODIFY
        	kfree(data);
#endif
		return ret;
    }
   
    //dev_err(&func->dev, "sdio read reg device[%08x] parent[%08x]\n",child,child->parent);

    if ( glue != NULL )
    {
        func = dev_to_sdio_func(glue->dev);
        sdio_claim_host(func);

        // 4 bytes address
        data[0] = (addr >> ( 0 )) &0xff;
        data[1] = (addr >> ( 8 )) &0xff;
        data[2] = (addr >> ( 16 )) &0xff;
        data[3] = (addr >> ( 24 )) &0xff;

        //8 byte ( 4 bytes address , 4 bytes data )
        ret = sdio_memcpy_toio(func, glue->regIOPort, data, 4);
        if (WARN_ON(ret))
        {
            dev_err(&func->dev, "sdio read reg write address failed (%d)\n", ret);
            goto io_err;
        }

        ret = sdio_memcpy_fromio(func, data, glue->regIOPort, 4);
        if (WARN_ON(ret))
        {
            dev_err(&func->dev, "sdio read reg from I/O failed (%d)\n",ret);
        	goto io_err;
    	 }

        if(ret == 0)
        {
            *buf = (data[0]&0xff);
            *buf = *buf | ((data[1]&0xff)<<( 8 ));
            *buf = *buf | ((data[2]&0xff)<<( 16 ));
            *buf = *buf | ((data[3]&0xff)<<( 24 ));
        }
        else
            *buf = 0xffffffff;
io_err:
        sdio_release_host(func);
        //dev_dbg(&func->dev, "sdio read reg addr 0x%x, 0x%x  ret:%d\n", addr, *buf, ret);
        CHECK_IO_RET(glue, ret);
    }
    else
    {
        dev_err(&func->dev, "sdio read reg glue == NULL!!!\n");
    }

    //if (WARN_ON(ret))
    //  dev_err(&func->dev, "sdio read reg failed (%d)\n", ret);

#if 1//#ifdef CONFIG_AMLOGIC_MODIFY
    kfree(data);
#endif
    return ret;
}

static void hwif_ctrl_read_parameter(struct sdio_func *func,
        struct hwif_ctrl_glue *glue)
{
    int err_ret;
    sdio_claim_host(func);

    //get dataIOPort(Accesee packet buffer & SRAM)
    glue->dataIOPort = 0;
    glue->dataIOPort = glue->dataIOPort | (sdio_readb(func, REG_DATA_IO_PORT_0, &err_ret) << ( 8*0 ));
    glue->dataIOPort = glue->dataIOPort | (sdio_readb(func, REG_DATA_IO_PORT_1, &err_ret) << ( 8*1 ));
    glue->dataIOPort = glue->dataIOPort | (sdio_readb(func, REG_DATA_IO_PORT_2, &err_ret) << ( 8*2 ));

    //get regIOPort(Access register)
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

    // output timing
#ifdef CONFIG_PLATFORM_SDIO_OUTPUT_TIMING
    sdio_writeb(func, CONFIG_PLATFORM_SDIO_OUTPUT_TIMING,REG_OUTPUT_TIMING_REG, &err_ret);
#else
    sdio_writeb(func, SDIO_DEF_OUTPUT_TIMING,REG_OUTPUT_TIMING_REG, &err_ret);
#endif

    // switch to normal mode
    // bit[1] , 0:normal mode, 1: Download mode(For firmware download & SRAM access)
    sdio_writeb(func, 0x00,REG_Fn1_STATUS, &err_ret);

#if 0
    //to check if support tx alloc mechanism
    sdio_writeb(func,SDIO_TX_ALLOC_SIZE_SHIFT|SDIO_TX_ALLOC_ENABLE,REG_SDIO_TX_ALLOC_SHIFT, &err_ret);
#endif

    sdio_release_host(func);
}

static int hwif_ctrl_power_on(struct sdio_func *func)
{
	
	int ret = 0;

    printk("hwif_ctrl_power_on\n");

	sdio_claim_host(func);
	ret = sdio_enable_func(func);
	sdio_release_host(func);
	
	
	if (ret) {
		printk("Unable to enable sdio func: %d)\n", ret);		
		return ret;
	}
	
	/*
	 * Wait for hardware to initialise. It should take a lot less than
	 * 10 ms but let's be conservative here.
	 */
	msleep(10);

	return ret;
}

/*
static void hwif_ctrl_direct_int_mux_mode(struct hwif_ctrl_glue *glue, bool enable)
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
*/

//static int hwif_ctrl_power_off(struct sdio_func *func)
//{
//	int ret;
//
//    printk("hwif_ctrl_power_off\n");
//
//	/* Disable the card */
//	sdio_claim_host(func);
//	ret = sdio_disable_func(func);
//	sdio_release_host(func);
//
//	if (ret)
//		return ret;
//
//	return ret;
//}

static void _read_chip_id (struct hwif_ctrl_glue *glue)
{
    u32 regval;
    int ret;
    u8 _chip_id[SSV6XXX_CHIP_ID_LENGTH];
    u8 *c = _chip_id;
    int i = 0;

    //CHIP ID
    // Chip ID registers should be common to all SSV6xxx devices. So these registers 
    // must not come from turismo_reg.h but defined somewhere else.
    ret = __hwif_ctrl_read_reg(glue, ADR_CHIP_ID_3, &regval);
    *((u32 *)&_chip_id[0]) = __be32_to_cpu(regval);

    if (ret == 0)
        ret = __hwif_ctrl_read_reg(glue, ADR_CHIP_ID_2, &regval);
    *((u32 *)&_chip_id[4]) = __be32_to_cpu(regval);

    if (ret == 0)
        ret = __hwif_ctrl_read_reg(glue, ADR_CHIP_ID_1, &regval);
    *((u32 *)&_chip_id[8]) = __be32_to_cpu(regval);

    if (ret == 0)
        ret = __hwif_ctrl_read_reg(glue, ADR_CHIP_ID_0, &regval);
    *((u32 *)&_chip_id[12]) = __be32_to_cpu(regval);

    _chip_id[12+sizeof(u32)] = 0;

    // skip null for turimo fpga chip_id bug)
    while (*c == 0) {
        i++;
        c++;
        if (i == 16) { // max string length reached.
            c = _chip_id;
            break;
        }
    }

    if (*c != 0) {
        strncpy(glue->tmp_data.chip_id, c, SSV6XXX_CHIP_ID_LENGTH);
        //strncpy(glue->chip_id, c, SSV6XXX_CHIP_ID_LENGTH);
        glue->tmp_data.chip_id[SSV6XXX_CHIP_ID_LENGTH] = 0;
        //glue->chip_id[SSV6XXX_CHIP_ID_LENGTH] = 0;
        dev_info(glue->dev, "CHIP ID: %s \n", glue->tmp_data.chip_id);
        //dev_info(glue->dev, "CHIP ID: %s \n", glue->chip_id);
        strncpy(glue->tmp_data.short_chip_id, c, SSV6XXX_CHIP_ID_SHORT_LENGTH);
        //strncpy(glue->short_chip_id, c, SSV6XXX_CHIP_ID_SHORT_LENGTH);
        glue->tmp_data.short_chip_id[SSV6XXX_CHIP_ID_SHORT_LENGTH] = 0;
        //glue->short_chip_id[SSV6XXX_CHIP_ID_SHORT_LENGTH] = 0;
        dev_info(glue->dev, "SHORT CHIP ID: %s \n", glue->tmp_data.short_chip_id);
        //dev_info(glue->dev, "SHORT CHIP ID: %s \n", glue->short_chip_id);
    } else {
        dev_err(glue->dev, "Failed to read chip ID");
        glue->tmp_data.chip_id[0] = 0;
        glue->tmp_data.short_chip_id[0] = 0;
        //glue->chip_id[0] = 0;
        //glue->short_chip_id[0] = 0;
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
        //init delay value
        delay[i] = 0;

        in_delay = (input_delay >> ( i * 8 )) & 0xff;
        out_delay = (output_delay >> ( i * 8 )) & 0xff;

        //set delay value
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

    //set sdio delay value
    sdio_claim_host(func);
    sdio_writeb(func, delay[0], REG_SDIO_DAT0_DELAY, &ret);
    sdio_writeb(func, delay[1], REG_SDIO_DAT1_DELAY, &ret);
    sdio_writeb(func, delay[2], REG_SDIO_DAT2_DELAY, &ret);
    sdio_writeb(func, delay[3], REG_SDIO_DAT3_DELAY, &ret);
	sdio_release_host(func);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
static int __devinit hwif_ctrl_probe(struct sdio_func *func,
        const struct sdio_device_id *id)
#else
static int hwif_ctrl_probe(struct sdio_func *func,
        const struct sdio_device_id *id)
#endif
{
    //struct turismo_platform_data *pwlan_data;
    struct hwif_ctrl_glue     glue;

    //struct resource res[1];
    //mmc_pm_flag_t mmcflags;
    int ret = -ENOMEM;

    printk(KERN_INFO "=======================================\n");
    printk(KERN_INFO "==           RUN HWIF CTRL           ==\n");
    printk(KERN_INFO "=======================================\n");
    

    /* We are only able to handle the wlan function */
    if (func->num != 0x01)
        return -ENODEV;

    /* 
     * Setting SDIO delay chain
     * Note: delay chain function cannot work in CABRIO 
     */
#if (defined(CONFIG_SSV_SDIO_INPUT_DELAY) && defined(CONFIG_SSV_SDIO_OUTPUT_DELAY))
    ssv6xxx_sdio_delay_chain(func, CONFIG_SSV_SDIO_INPUT_DELAY, CONFIG_SSV_SDIO_OUTPUT_DELAY);
#endif

    //pwlan_data = &glue->tmp_data;
    //memset(pwlan_data, 0, sizeof(struct turismo_platform_data));
   
    glue.dev = &func->dev;
    //init_waitqueue_head(&glue->irq_wq);

    /* Grab access to FN0 for ELP reg. */
    func->card->quirks |= MMC_QUIRK_LENIENT_FN0;

    /* Use block mode for transferring over one block size of data */
    func->card->quirks |= MMC_QUIRK_BLKSZ_FOR_BYTE_MODE;

    /* if sdio can keep power while host is suspended, enable wow */
    //mmcflags = sdio_get_host_pm_caps(func);
    //dev_err(glue->dev, "sdio PM caps = 0x%x\n", mmcflags);
/*
    if (mmcflags & MMC_PM_KEEP_POWER)
        pwlan_data->pwr_in_suspend = true;
*/
    //store sdio vendor/device id
    //pwlan_data->vendor = func->vendor;
    //pwlan_data->device = func->device;

    //dev_err(glue->dev, "vendor = 0x%x device = 0x%x\n",
    //        pwlan_data->vendor, pwlan_data->device);

#ifdef CONFIG_PM
    /*
        Some platforms LDO-EN can not be pulled down. WIFI cause leakage.
        Avoid leakage issues
    */
#if 0
    turismo_do_sdio_wakeup(func);
#endif
#endif

    hwif_ctrl_power_on(func);
    
    hwif_ctrl_read_parameter(func, &glue);

    //hwif_ctrl_direct_int_mux_mode(&glue, false);
    //turismo_set_bus_width(func, MMC_BUS_WIDTH_4);

    /* Tell PM core that we don't need the card to be powered now */
    //pm_runtime_put_noidle(&func->dev);
    _read_chip_id(&glue);

    if (   strstr(glue.tmp_data.chip_id, SSV6051_CHIP)
        || strstr(glue.tmp_data.chip_id, SSV6051_CHIP_ECO3)) {
        sdio_product = CABRIO_SDIO_DEV;
        ret = ssv6xxx_sdio_probe(func, id);
        goto out;
    }
    sdio_product = TURISMO_SDIO_DEV;
    ret = tu_ssv6xxx_sdio_probe(func, id);

out:
    return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
static void __devexit hwif_ctrl_remove(struct sdio_func *func)
#else
static void hwif_ctrl_remove(struct sdio_func *func)
#endif
{
    printk("hwif_ctrl_remove..........\n");

    if (sdio_product == CABRIO_SDIO_DEV) {
        ssv6xxx_sdio_remove(func);
        return;
    }
    tu_ssv6xxx_sdio_remove(func);

    printk("hwif_ctrl_remove leave..........\n");
}

#ifdef CONFIG_PM
static int hwif_ctrl_suspend(struct device *dev)
{
    if (sdio_product == CABRIO_SDIO_DEV) {
        return ssv6xxx_sdio_suspend(dev);
    }
    return tu_ssv6xxx_sdio_suspend(dev);
}


static int hwif_ctrl_resume(struct device *dev)
{
    if (sdio_product == CABRIO_SDIO_DEV) {
        return ssv6xxx_sdio_resume(dev);
    }
    return tu_ssv6xxx_sdio_resume(dev);
}

static const struct dev_pm_ops hwif_ctrl_pm_ops =
{
    .suspend    = hwif_ctrl_suspend,
    .resume     = hwif_ctrl_resume,
};
#endif

struct sdio_driver hwif_ctrl_driver =
{
    .name		= "SSV_HWIF_CTRL",
    .id_table	= hwif_ctrl_devices,
    .probe		= hwif_ctrl_probe,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
    .remove		= __devexit_p(hwif_ctrl_remove),
#else
    .remove     = hwif_ctrl_remove,
#endif
    
#ifdef CONFIG_PM
    .drv = {
        .pm = &hwif_ctrl_pm_ops,
    },
#endif
};
EXPORT_SYMBOL(hwif_ctrl_driver);

#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
int hwif_ctrl_init(void)
#else
static int __init hwif_ctrl_init(void)
#endif
{
    printk(KERN_INFO "hwif_ctrl_init\n");
    return sdio_register_driver(&hwif_ctrl_driver);
}

#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
void hwif_ctrl_exit(void)
#else
static void __exit hwif_ctrl_exit(void)
#endif
{
    printk(KERN_INFO "hwif_ctrl_exit\n");
    sdio_unregister_driver(&hwif_ctrl_driver);
}
#if 0//#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
EXPORT_SYMBOL(hwif_ctrl_init);
EXPORT_SYMBOL(hwif_ctrl_exit);
#else
module_init(hwif_ctrl_init);
module_exit(hwif_ctrl_exit);
#endif
MODULE_LICENSE("GPL");
