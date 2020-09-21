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

#include <linux/irq.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pm_runtime.h>
#include <linux/mmc/host.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/regulator/consumer.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <mach/sys_config.h>
#include <linux/power/scenelock.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
#include <linux/printk.h>
#include <linux/err.h>
#else
#include <config/printk.h>
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,2,0)
#include <linux/wlan_plat.h>
#else
struct wifi_platform_data {
    int (*set_power)(int val);
    int (*set_reset)(int val);
    int (*set_carddetect)(int val);
    void *(*mem_prealloc)(int section, unsigned long size);
    int (*get_mac_addr)(unsigned char *buf);
    void *(*get_country_code)(char *ccode);
};
#endif
#ifdef CONFIG_HAS_WAKELOCK
struct wake_lock icomm_wake_lock;
#endif
static int g_wifidev_registered = 0;
static struct semaphore wifi_control_sem;
static struct wifi_platform_data *wifi_control_data = NULL;
#ifdef SSV_WAKEUP_HOST
static unsigned int g_wifi_irq_rc = 0;
#endif
static int sdc_id = -1;
#define SDIO_ID 1
#define IRQ_RES_NAME "ssv_wlan_irq"
#define WIFI_HOST_WAKE 0xFFFF
extern void sunxi_mci_rescan_card(unsigned id, unsigned insert);
extern int wifi_pm_gpio_ctrl(char *name, int level);
extern int enable_wakeup_src(cpu_wakeup_src_e src, int para);
extern int disable_wakeup_src(cpu_wakeup_src_e src, int para);
extern void wifi_pm_power(int on);
static int ssv_wifi_power(int on)
{
    printk("ssv pwr on=%d\n", on);
    if (on) {
        wifi_pm_power(0);
        mdelay(50);
        wifi_pm_power(1);
    } else {
        wifi_pm_power(0);
    }
    return 0;
}
static int ssv_wifi_reset(int on)
{
    return 0;
}
int ssv_wifi_set_carddetect(int val)
{
    sunxi_mci_rescan_card(sdc_id, val);
    return 0;
}
static struct wifi_platform_data ssv_wifi_control = {
    .set_power = ssv_wifi_power,
    .set_reset = ssv_wifi_reset,
    .set_carddetect = ssv_wifi_set_carddetect,
};
static struct resource resources[] = {
    {
        .start = WIFI_HOST_WAKE,
        .flags = IORESOURCE_IRQ,
        .name = IRQ_RES_NAME,
    },
};
void ssv_wifi_device_release(struct device *dev)
{
    printk(KERN_INFO "ssv_wifi_device_release\n");
}
static struct platform_device ssv_wifi_device = {
    .name = "ssv_wlan",
    .id = 1,
    .num_resources = ARRAY_SIZE(resources),
    .resource = resources,
    .dev = {
            .platform_data = &ssv_wifi_control,
            .release = ssv_wifi_device_release,
     },
};
int wifi_set_power(int on, unsigned long msec)
{
    if (wifi_control_data && wifi_control_data->set_power) {
        wifi_control_data->set_power(on);
    }
    if (msec)
        msleep(msec);
    return 0;
}
int wifi_set_reset(int on, unsigned long msec)
{
    if (wifi_control_data && wifi_control_data->set_reset) {
        wifi_control_data->set_reset(on);
    }
    if (msec)
        msleep(msec);
    return 0;
}
static int wifi_set_carddetect(int on)
{
    if (wifi_control_data && wifi_control_data->set_carddetect) {
        wifi_control_data->set_carddetect(on);
    }
    return 0;
}
#ifdef SSV_WAKEUP_HOST
static irqreturn_t wifi_wakeup_irq_handler(int irq, void *dev)
{
    wake_lock_timeout(&icomm_wake_lock, HZ);
    printk("***** %s ******\n", __func__);
    return IRQ_HANDLED;
}
void setup_wifi_wakeup_BB(struct platform_device *pdev, bool bEnable)
{
    int ret = 0;
    unsigned int oob_irq;
    unsigned gpio_eint_wlan = 0;
    bool enable = bEnable;
    script_item_u val;
    script_item_value_type_e type;
    int wakeup_src_cnt = 0;
    script_item_u *list = NULL;
    wakeup_src_cnt = script_get_pio_list("wakeup_src_para", &list);
    pr_err("wakeup src cnt is : %d. \n", wakeup_src_cnt);
    type = script_get_item("wifi_para", "ssv6051_wl_host_wake", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type || !enable) {
        printk("No definition of wake up host PIN\n");
        enable = false;
    } else {
        printk("WiFi wake up host PIN:%d=0x%x\n", val.gpio.gpio, val.gpio.gpio);
        gpio_eint_wlan = val.gpio.gpio;
        oob_irq = gpio_to_irq(gpio_eint_wlan);
    }
    if (enable) {
        printk("%s: enable irq\n", __FUNCTION__);
        g_wifi_irq_rc = oob_irq;
        ret = request_threaded_irq(
            oob_irq, NULL, (void *)wifi_wakeup_irq_handler,
            IRQ_TYPE_EDGE_FALLING,
            "wlan_wakeup_irq", NULL);
        enable_irq_wake(g_wifi_irq_rc);
    } else {
        if (g_wifi_irq_rc) {
            free_irq(g_wifi_irq_rc, NULL);
            g_wifi_irq_rc = 0;
        }
    }
}
#endif
static int wifi_probe(struct platform_device *pdev)
{
    script_item_u val;
    script_item_value_type_e type;
    int ret = 0;
    struct wifi_platform_data *wifi_ctrl =
        (struct wifi_platform_data *)(pdev->dev.platform_data);
    printk(KERN_ALERT "wifi_probe\n");
    wifi_control_data = wifi_ctrl;
    type = script_get_item("wifi_para", "wifi_sdc_id", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT!=type) {
        printk("get wifi_sdc_id failed\n");
        ret = -1;
    } else {
        sdc_id = val.val;
    }
    wifi_set_power(0, 40);
    wifi_set_power(1, 50);
    wifi_set_carddetect(1);
#ifdef SSV_WAKEUP_HOST
    setup_wifi_wakeup_BB(pdev, true);
#endif
    up(&wifi_control_sem);
    return ret;
}
static int wifi_remove(struct platform_device *pdev)
{
    struct wifi_platform_data *wifi_ctrl =
        (struct wifi_platform_data *)(pdev->dev.platform_data);
    wifi_control_data = wifi_ctrl;
#ifdef SSV_WAKEUP_HOST
    setup_wifi_wakeup_BB(pdev, false);
#endif
    wifi_set_power(0, 0);
    wifi_set_power(0, 0);
    wifi_set_power(0, 0);
    wifi_set_power(0, 0);
    wifi_set_carddetect(0);
    return 0;
}
static int wifi_suspend(struct platform_device *pdev, pm_message_t state)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}
static int wifi_resume(struct platform_device *pdev)
{
    printk("%s\n", __FUNCTION__);
    sunxi_mci_rescan_card(sdc_id, 1);
    wake_lock_timeout(&icomm_wake_lock, 5 * HZ);
    return 0;
}
static struct platform_driver wifi_driver = {
    .probe = wifi_probe,
    .remove = wifi_remove,
    .suspend = wifi_suspend,
    .resume = wifi_resume,
    .driver = {
    .name = "ssv_wlan",
    }
};
extern int tu_ssvdevice_init(void);
extern void tu_ssvdevice_exit(void);
#ifdef CONFIG_SSV_SUPPORT_AES_ASM
extern int aes_init(void);
extern void aes_fini(void);
extern int sha1_mod_init(void);
extern void sha1_mod_fini(void);
#endif
int initWlan(void)
{
    int ret = 0;
#ifdef CONFIG_HAS_WAKELOCK
    wake_lock_init(&icomm_wake_lock, WAKE_LOCK_SUSPEND, "ssv6051");
    wake_lock(&icomm_wake_lock);
#endif
    sema_init(&wifi_control_sem, 0);
#ifdef CONFIG_SSV_SUPPORT_AES_ASM
    sha1_mod_init();
    aes_init();
#endif
    platform_device_register(&ssv_wifi_device);
    platform_driver_register(&wifi_driver);
    g_wifidev_registered = 1;
    if (down_timeout(&wifi_control_sem, msecs_to_jiffies(1000)) != 0) {
        ret = -EINVAL;
        printk(KERN_ALERT "%s: platform_driver_register timeout\n", __FUNCTION__);
    }
    ret = tu_ssvdevice_init();
#ifdef CONFIG_HAS_WAKELOCK
    wake_unlock(&icomm_wake_lock);
#endif
    return ret;
}
void exitWlan(void)
{
    if (g_wifidev_registered)
    {
        tu_ssvdevice_exit();
#ifdef CONFIG_SSV_SUPPORT_AES_ASM
        aes_fini();
        sha1_mod_fini();
#endif
        platform_driver_unregister(&wifi_driver);
        platform_device_unregister(&ssv_wifi_device);
        g_wifidev_registered = 0;
    }
#ifdef CONFIG_HAS_WAKELOCK
    wake_lock_destroy(&icomm_wake_lock);
#endif
    return;
}
static int tu_generic_wifi_init_module(void)
{
    return initWlan();
}
static void tu_generic_wifi_exit_module(void)
{
    exitWlan();
}
EXPORT_SYMBOL(tu_generic_wifi_init_module);
EXPORT_SYMBOL(tu_generic_wifi_exit_module);
module_init(tu_generic_wifi_init_module);
module_exit(tu_generic_wifi_exit_module);
MODULE_LICENSE("Dual BSD/GPL");
