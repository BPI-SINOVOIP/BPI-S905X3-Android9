#include <linux/irq.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/gpio.h>
#include <linux/mmc/host.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>

//#include <mach/hardware.h>
#include <asm/io.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
#include <linux/printk.h>
#include <linux/err.h>
#else
#include <config/printk.h>
#endif
//#define USE_IMMEDIATE  

#define GPIO_REG_WRITEL(val, reg)    do{__raw_writel(val, CTL_PIN_BASE + (reg));}while(0)
extern void mmc_rescan_sdio(void);
extern void wifi_power(int on);
static int g_wifidev_registered = 0;

extern int ssvdevice_init(void);
extern void ssvdevice_exit(void);
#ifdef CONFIG_SSV_SUPPORT_AES_ASM
extern int aes_init(void);
extern void aes_fini(void);
extern int sha1_mod_init(void);
extern void sha1_mod_fini(void);
#endif
int initWlan(void)
{
    int ret=0;
    wifi_power(0);
    mdelay(10);
    wifi_power(1);
    mdelay(120);

	mmc_rescan_sdio();
    mdelay(120);

    g_wifidev_registered = 1;
	
    ret = ssvdevice_init();
    return ret;
}

void exitWlan(void)
{
    if (g_wifidev_registered)
    {
        ssvdevice_exit();
        wifi_power(0);

        g_wifidev_registered = 0;
    }
    return;
}
static __init int generic_wifi_init_module(void)
//int rockchip_wifi_init_module_ssv6xxx(void)
{
	printk("%s\n", __func__);
#ifdef CONFIG_SSV_SUPPORT_AES_ASM
	sha1_mod_init();
	aes_init();
#endif
	return initWlan();
}

static __exit void generic_wifi_exit_module(void)
//void rockchip_wifi_exit_module_ssv6xxx(void)
{
	printk("%s\n", __func__);
#ifdef CONFIG_SSV_SUPPORT_AES_ASM
        aes_fini();
        sha1_mod_fini();
#endif
	exitWlan();
}
//EXPORT_SYMBOL(rockchip_wifi_init_module_ssv6xxx);
//EXPORT_SYMBOL(rockchip_wifi_exit_module_ssv6xxx);
EXPORT_SYMBOL(generic_wifi_init_module);
EXPORT_SYMBOL(generic_wifi_exit_module);
module_init(generic_wifi_init_module);
module_exit(generic_wifi_exit_module);
MODULE_LICENSE("Dual BSD/GPL");

