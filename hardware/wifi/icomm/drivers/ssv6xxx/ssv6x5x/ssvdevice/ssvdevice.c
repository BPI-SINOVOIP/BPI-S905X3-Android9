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
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif
#include "ssv_cmd.h"
#include "ssv_cfg.h"
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/ctype.h>
#include <ssv6200.h>
#include <hci/hctrl.h>
#include <smac/dev.h>
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
#include <hci/ssv_hci.h>
#include <smac/init.h>
#include <hwif/sdio/sdio.h>
#include <hwif/usb/usb.h>
#endif
MODULE_AUTHOR("iComm-semi, Ltd");
MODULE_DESCRIPTION("Shared library for SSV wireless LAN cards.");
MODULE_LICENSE("Dual BSD/GPL");
static char *tu_stacfgpath = "/vendor/etc/wifi/ssv6x5x/ssv6x5x-wifi.cfg";
EXPORT_SYMBOL(tu_stacfgpath);
module_param(tu_stacfgpath, charp, 0000);
MODULE_PARM_DESC(tu_stacfgpath, "Get path of sta cfg");
char *tu_cfgfirmwarepath = NULL;
EXPORT_SYMBOL(tu_cfgfirmwarepath);
module_param(tu_cfgfirmwarepath, charp, 0000);
MODULE_PARM_DESC(tu_cfgfirmwarepath, "Get firmware path");
char* tu_ssv_initmac = NULL;
EXPORT_SYMBOL(tu_ssv_initmac);
module_param(tu_ssv_initmac, charp, 0644);
MODULE_PARM_DESC(tu_ssv_initmac, "Wi-Fi MAC address");
int ssv_rx_nr_recvbuff = 2;
EXPORT_SYMBOL(ssv_rx_nr_recvbuff);
module_param(ssv_rx_nr_recvbuff, int, 0644);
MODULE_PARM_DESC(ssv_rx_nr_recvbuff, "USB RX buffer 1 ~ MAX_NR_RECVBUFF");
int ssv_rx_use_wq = 0;
EXPORT_SYMBOL(ssv_rx_use_wq);
module_param(ssv_rx_use_wq, int, 0444);
MODULE_PARM_DESC(ssv_rx_use_wq, "USB RX uses workqueue instead of tasklet");
static struct proc_dir_entry *__ssv_procfs;
extern struct ssv6xxx_cfg_cmd_table tu_cfg_cmds[];
extern struct ssv6xxx_cfg tu_ssv_cfg;
#define READ_CHUNK 32
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#define PDE_DATA(inode) ({ \
    struct proc_dir_entry *dp = PDE(inode); \
    data = dp->data; })
#endif
static char *p2pStatus = "0";
static int ssv6xxx_p2p_open(struct inode *inode, struct file *filp)
{
    void *data = PDE_DATA(inode);
    filp->private_data = data;
    return 0;
}
static ssize_t ssv6xxx_p2p_read(struct file *filp, char __user *buffer,
                size_t count, loff_t *ppos)
{
    s32 ret, retlen, retval = 0;
    retlen = strlen(p2pStatus)+1;
    if (*ppos >= retlen)
        goto out;
    if (*ppos + count > retlen)
        retlen = retlen - *ppos;
    ret = copy_to_user(buffer, p2pStatus, retlen);
    printk("%s: p2pStatus = %s \n", __func__,p2pStatus);
    *ppos += retlen - ret;
    retval = retlen - ret;
out:
    return retval;
}
static ssize_t ssv6xxx_p2p_write(struct file *filp, const char __user *buffer,
     size_t count, loff_t *ppos)
{
    struct ssv_cmd_data *cmd_data = filp->private_data;
    struct ssv_softc *sc = container_of(cmd_data, struct ssv_softc, cmd_data);
 int enable = 0;
 ssize_t retval= -ENOMEM;
 if (*ppos != 0 || count > 255)
  return 0;
 if (copy_from_user(p2pStatus, buffer, count)){
  retval = -EFAULT;
  goto out;
 }
 p2pStatus[count - 1] = 0;
 enable = simple_strtol(p2pStatus, NULL, 10);
 printk("%s: enable = %d\n", __func__, enable);
 if(enable >= 0){
  sc->p2p_status = enable;
  printk("p2p_status:%d\n",sc->p2p_status);
 }
 retval = count;
out:
    return retval;
}
static struct file_operations ssv6xxx_p2p_fops = {
    .owner = THIS_MODULE,
    .open = ssv6xxx_p2p_open,
    .read = ssv6xxx_p2p_read,
    .write = ssv6xxx_p2p_write,
};
static int ssv6xxx_freq_open(struct inode *inode, struct file *filp)
{
    void *data = PDE_DATA(inode);
    filp->private_data = data;
    return 0;
}
static ssize_t ssv6xxx_freq_read(struct file *filp, char __user *buffer,
                size_t count, loff_t *ppos)
{
    struct ssv_cmd_data *cmd_data = filp->private_data;
    struct ssv_softc *sc = container_of(cmd_data, struct ssv_softc, cmd_data);
    char chan_freq[8]="0";
    s32 ret, retlen, retval = 0;
    u16 channel_freq = sc->channel_center_freq;
    if(channel_freq > 0)
        sprintf(chan_freq, "%u", channel_freq);
    retlen = strlen(chan_freq)+1;
    if (*ppos >= retlen)
        goto out;
    if (*ppos + count > retlen)
        retlen = retlen - *ppos;
    printk("freq=%s\n", chan_freq);
    ret = copy_to_user(buffer, &chan_freq, retlen);
    *ppos += retlen - ret;
    retval = retlen - ret;
out:
    return retval;
}
static struct file_operations ssv6xxx_freq_fops = {
    .owner = THIS_MODULE,
    .open = ssv6xxx_freq_open,
    .read = ssv6xxx_freq_read,
};
static int ssv6xxx_cmd_file_open(struct inode *inode, struct file *filp)
{
    void *data = PDE_DATA(inode);
    filp->private_data = data;
    return 0;
}
static ssize_t ssv6xxx_cmd_file_read(struct file *filp, char __user *buffer,
                                size_t count, loff_t *ppos)
{
    char *ssv6xxx_result_buf;
    struct ssv_cmd_data *cmd_data = filp->private_data;
    int len;
    int ret = 0;
    if (!(cmd_data->cmd_in_proc)){
        goto out;
    }
    ssv6xxx_result_buf = cmd_data->ssv6xxx_result_buf;
    cmd_data->cmd_in_proc = false;
    if (!ssv6xxx_result_buf){
        goto out;
    }
    cmd_data->ssv6xxx_result_buf = NULL;
    if (*ppos != 0){
        goto free;
    }
    if (cmd_data->rsbuf_size < cmd_data->rsbuf_len)
        cmd_data->rsbuf_len = cmd_data->rsbuf_size-1;
    len = cmd_data->rsbuf_len + 1;
    if (len == 1){
        goto free;
    }
    if (copy_to_user(buffer, ssv6xxx_result_buf, len)){
        ret = -EFAULT;
        goto free;
    }
    ret = len;
free:
    kfree(ssv6xxx_result_buf);
out:
    if (atomic_read (&cmd_data->cli_count) > 0)
        atomic_dec(&cmd_data->cli_count);
    return ret;
}
static ssize_t ssv6xxx_cmd_file_write(struct file *filp, const char __user *buffer,
                size_t count, loff_t *ppos)
{
    char *ssv6xxx_cmd_buf = NULL;
    struct ssv_cmd_data *cmd_data = filp->private_data;
    int i = 0;
    if (*ppos != 0 || count > 255)
        return 0;
    ssv6xxx_cmd_buf = (char *)kzalloc(count, GFP_KERNEL);
    if (!ssv6xxx_cmd_buf)
        return 0;
    ssv6xxx_cmd_buf[0] = 0x00;
    if (copy_from_user(ssv6xxx_cmd_buf, buffer, count))
        return -EFAULT;
    while (atomic_read (&cmd_data->cli_count) != 0) {
        msleep(1);
        i++;
        if (i > 1000) return -EFAULT;
    }
    atomic_add(2, &cmd_data->cli_count);
    ssv6xxx_cmd_buf[count-1] = 0x00;
    ssv_cmd_submit((struct ssv_cmd_data *)filp->private_data, ssv6xxx_cmd_buf);
    kfree(ssv6xxx_cmd_buf);
    return count;
}
size_t read_line(struct file *fp, char *buf, size_t size)
{
 size_t num_read = 0;
 size_t total_read = 0;
 char *buffer;
 char ch;
 size_t start_ignore = 0;
 if (size <= 0 || buf == NULL) {
  total_read = -EINVAL;
  return -EINVAL;
 }
 buffer = buf;
 for (;;) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,4,37)
  if (fp->f_op && fp->f_op->read)
   num_read = fp->f_op->read(fp, &ch, 1, &fp->f_pos);
#else
  num_read = vfs_read(fp, &ch, 1, &fp->f_pos);
#endif
  if (num_read < 0) {
   if (num_read == EINTR)
    continue;
   else
    return -1;
  }
  else if (num_read == 0) {
   if (total_read == 0)
    return 0;
   else
    break;
  }
  else {
   if (ch == '#')
    start_ignore = 1;
   if (total_read < size - 1) {
    total_read++;
    if (start_ignore)
     *buffer++ = '\0';
    else
     *buffer++ = ch;
   }
   if (ch == '\n')
    break;
  }
 }
 *buffer = '\0';
 return total_read;
}
int ischar(char *c)
{
 int is_char = 1;
 while(*c) {
  if (isalpha(*c) || isdigit(*c) || *c == '_' || *c == ':' || *c == '/' || *c == '.' || *c == '-')
   c++;
  else {
   is_char = 0;
   break;
  }
 }
 return is_char;
}
static void _set_initial_cfg_default(void)
{
    size_t s;
  for(s=0; tu_cfg_cmds[s].cfg_cmd != NULL; s++) {
  if ((tu_cfg_cmds[s].def_val)!= NULL) {
   tu_cfg_cmds[s].translate_func(tu_cfg_cmds[s].def_val,
    tu_cfg_cmds[s].var, tu_cfg_cmds[s].arg);
  }
 }
}
static void _import_default_cfg (char *tu_stacfgpath)
{
 struct file *fp = (struct file *) NULL;
 char buf[MAX_CHARS_PER_LINE], cfg_cmd[32], cfg_value[32];
 mm_segment_t fs;
 size_t s, read_len = 0, is_cmd_support = 0;
 printk("\n*** %s, %s ***\n\n", __func__, tu_stacfgpath);
 if (tu_stacfgpath == NULL)
  return;
 memset(&tu_ssv_cfg, 0, sizeof(tu_ssv_cfg));
 memset(buf, 0, sizeof(buf));
 _set_initial_cfg_default();
 fp = filp_open(tu_stacfgpath, O_RDONLY, 0);
 if (IS_ERR(fp) || fp == NULL) {
  printk("ERROR: filp_open\n");
        WARN_ON(1);
  return;
 }
 if (fp->f_path.dentry == NULL) {
  printk("ERROR: dentry NULL\n");
        WARN_ON(1);
  return;
 }
 do {
  memset(cfg_cmd, '\0', sizeof(cfg_cmd));
  memset(cfg_value, '\0', sizeof(cfg_value));
  fs = get_fs();
  set_fs(get_ds());
  read_len = read_line(fp, buf, MAX_CHARS_PER_LINE);
  set_fs(fs);
  sscanf(buf, "%s = %s", cfg_cmd, cfg_value);
  if (!ischar(cfg_cmd) || !ischar(cfg_value)) {
   printk("ERORR invalid parameter: %s\n", buf);
   WARN_ON(1);
   continue;
  }
  is_cmd_support = 0;
  for(s=0; tu_cfg_cmds[s].cfg_cmd != NULL; s++) {
   if (strcmp(tu_cfg_cmds[s].cfg_cmd, cfg_cmd)==0) {
    tu_cfg_cmds[s].translate_func(cfg_value,
     tu_cfg_cmds[s].var, tu_cfg_cmds[s].arg);
    is_cmd_support = 1;
    break;
   }
  }
  if (!is_cmd_support && strlen(cfg_cmd) > 0) {
   printk("ERROR Unsupported command: %s", cfg_cmd);
   WARN_ON(1);
  }
 } while (read_len > 0);
 filp_close(fp, NULL);
}
static struct file_operations ssv6xxx_cmd_fops = {
    .owner = THIS_MODULE,
    .open = ssv6xxx_cmd_file_open,
    .read = ssv6xxx_cmd_file_read,
    .write = ssv6xxx_cmd_file_write,
};
static void *ssv6xxx_dbg_seq_start(struct seq_file *s, loff_t *pos)
{
 struct ssv_cmd_data *cmd_data = s->private;
 struct ssv_dbg_log *dbg_log = &cmd_data->dbg_log;
 *pos = 0;
 rcu_read_lock();
 if (dbg_log->size <= 0)
  return NULL;
 return dbg_log;
}
static void *ssv6xxx_dbg_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
 struct ssv_cmd_data *cmd_data = s->private;
 struct ssv_dbg_log *dbg_log = &cmd_data->dbg_log;
 ++*pos;
 if (dbg_log->size <= 0)
  return NULL;
 return dbg_log;
}
static void ssv6xxx_dbg_seq_stop(struct seq_file *s, void *v)
{
 struct ssv_cmd_data *cmd_data = s->private;
 struct ssv_dbg_log *dbg_log = &cmd_data->dbg_log;
 if ((dbg_log == NULL) || (dbg_log->data == NULL) || (dbg_log->totalsize == 0)) {
  rcu_read_lock();
  return;
 }
 if (dbg_log->size == 0) {
  dbg_log->top = dbg_log->data;
  dbg_log->tail = dbg_log->data;
  dbg_log->end = &(dbg_log->data[dbg_log->totalsize]);
 }
 seq_putc(s, '\n');
 rcu_read_lock();
 return;
}
static int ssv6xxx_dbg_seq_show(struct seq_file *s, void *v)
{
 struct ssv_dbg_log *dbg_log = (struct ssv_dbg_log *)v;
 char *p = dbg_log->top;
 seq_putc(s, *p++);
 if (p == dbg_log->end)
  p = dbg_log->data;
 dbg_log->top = p;
 dbg_log->size--;
 return 0;
}
static struct seq_operations ssv6xxx_dbg_seq_fops = {
 .start = ssv6xxx_dbg_seq_start,
 .next = ssv6xxx_dbg_seq_next,
 .stop = ssv6xxx_dbg_seq_stop,
 .show = ssv6xxx_dbg_seq_show,
};
static int ssv6xxx_dbg_file_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
 struct seq_file *sf;
    void *data = PDE_DATA(inode);
 ret = seq_open(filp, &ssv6xxx_dbg_seq_fops);
 if (!ret) {
  sf = filp->private_data;
  sf->private = data;
 }
 return ret;
}
static struct file_operations ssv6xxx_dbg_fops = {
 .owner = THIS_MODULE,
 .open = ssv6xxx_dbg_file_open,
 .read = seq_read,
 .llseek = seq_lseek,
 .release = seq_release,
};
int ssv_init_cli (const char *dev_name, struct ssv_cmd_data *cmd_data)
{
    struct proc_dir_entry *proc_file_entry;
    cmd_data->proc_dev_entry = proc_mkdir(dev_name, __ssv_procfs);
    if (!cmd_data->proc_dev_entry)
        printk("KERN_ERR" "Failed to create %s dev directory for CLI. \n", dev_name);
    proc_file_entry = proc_create_data(PROC_SSV_CMD_ENTRY, S_IRUGO|S_IWUGO, cmd_data->proc_dev_entry, &ssv6xxx_cmd_fops, cmd_data);
    if (proc_file_entry == NULL)
        printk(KERN_ERR "Failed to create %s for CLI.\n", PROC_SSV_CMD_ENTRY);
    proc_file_entry = proc_create_data(PROC_SSV_DBG_ENTRY, S_IRUGO|S_IWUGO, cmd_data->proc_dev_entry, &ssv6xxx_dbg_fops, cmd_data);
    if (proc_file_entry == NULL)
        printk(KERN_ERR "Failed to create %s for SSV DBG.\n", PROC_SSV_DBG_ENTRY);
    proc_file_entry = proc_create_data(PROC_SSV_FREQ_ENTRY, S_IRUGO|S_IWUGO, cmd_data->proc_dev_entry, &ssv6xxx_freq_fops, cmd_data);
    if (proc_file_entry == NULL)
        printk(KERN_ERR "Failed to create %s for SSV FREQ.\n", PROC_SSV_FREQ_ENTRY);
    proc_file_entry = proc_create_data(PROC_SSV_P2P_ENTRY, S_IRUGO|S_IWUGO, cmd_data->proc_dev_entry, &ssv6xxx_p2p_fops, cmd_data);
    if (proc_file_entry == NULL)
        printk(KERN_ERR "Failed to create %s for SSV P2P.\n", PROC_SSV_P2P_ENTRY);
    atomic_set(&cmd_data->cli_count, 0);
    return 0;
}
EXPORT_SYMBOL(ssv_init_cli);
int ssv_deinit_cli (const char *dev_name, struct ssv_cmd_data *cmd_data)
{
    remove_proc_entry(PROC_SSV_DBG_ENTRY, cmd_data->proc_dev_entry);
    remove_proc_entry(PROC_SSV_CMD_ENTRY, cmd_data->proc_dev_entry);
    remove_proc_entry(PROC_SSV_FREQ_ENTRY, cmd_data->proc_dev_entry);
    remove_proc_entry(PROC_SSV_P2P_ENTRY, cmd_data->proc_dev_entry);
    remove_proc_entry(dev_name, __ssv_procfs);
    return 0;
}
EXPORT_SYMBOL(ssv_deinit_cli);
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
int tu_ssvdevice_init(void)
#else
static int __init tu_ssvdevice_init(void)
#endif
{
    _import_default_cfg(tu_stacfgpath);
    __ssv_procfs = proc_mkdir(PROC_DIR_ENTRY, NULL);
    if (!__ssv_procfs)
        return -ENOMEM;
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
    {
        int ret;
        ret = tu_ssv6xxx_hci_init();
        if(!ret){
            ret = tu_ssv6xxx_init();
        }if(!ret){
            ret = tu_ssv6xxx_sdio_init();
  #if (defined(SSV_SUPPORT_SSV6006))
            ret = ssv6xxx_usb_init();
  #endif
        }
        return ret;
    }
#endif
    return 0;
}
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
void tu_ssvdevice_exit(void)
#else
static void __exit tu_ssvdevice_exit(void)
#endif
{
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
    tu_ssv6xxx_exit();
    tu_ssv6xxx_hci_exit();
    tu_ssv6xxx_sdio_exit();
#if (defined(SSV_SUPPORT_SSV6006))
    ssv6xxx_usb_exit();
#endif
#endif
    remove_proc_entry(PROC_DIR_ENTRY, NULL);
}
#if 0//(defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
EXPORT_SYMBOL(tu_ssvdevice_init);
EXPORT_SYMBOL(tu_ssvdevice_exit);
#else
module_init(tu_ssvdevice_init);
module_exit(tu_ssvdevice_exit);
#endif
