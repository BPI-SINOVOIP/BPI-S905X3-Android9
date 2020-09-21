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
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <asm/io.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
#include <linux/printk.h>
#include <linux/err.h>
#else
#include <config/printk.h>
#endif
extern int tu_ssvdevice_init(void);
extern void tu_ssvdevice_exit(void);
int initWlan(void)
{
    int ret=0;
    printk(KERN_INFO "wlan.c initWlan\n");
    ret = tu_ssvdevice_init();
    return ret;
}
void exitWlan(void)
{
    tu_ssvdevice_exit();
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
#ifdef CONFIG_SSV6X5X
late_initcall(tu_generic_wifi_init_module);
#else
module_init(tu_generic_wifi_init_module);
#endif
module_exit(tu_generic_wifi_exit_module);
MODULE_LICENSE("Dual BSD/GPL");
