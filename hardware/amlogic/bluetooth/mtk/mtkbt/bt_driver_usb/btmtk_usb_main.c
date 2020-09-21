
/*
 *  Copyright (c) 2016~2018 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/completion.h>
#include <linux/usb.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/skbuff.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/kallsyms.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "btmtk_usb_main.h"

#if ENABLE_BT_FIFO_THREAD
#include "btmtk_usb_fifo.h"
#endif

/*============================================================================*/
/* Local Configuration */
/*============================================================================*/
#define VERSION "6.0.18112601"

/*============================================================================*/
/* Function Prototype */
/*============================================================================*/
#define ___________________________________________________Function_Prototype
static int btmtk_usb_standby(void);
static void btmtk_usb_cap_init(void);
static int btmtk_usb_BT_init(void);
static void btmtk_usb_BT_exit(void);
static int btmtk_usb_start_traffic(void);
static int btmtk_usb_send_assert_cmd(void);
static int btmtk_usb_send_assert_cmd_ctrl(void);
static int btmtk_usb_send_assert_cmd_bulk(void);
static int btmtk_usb_submit_intr_urb(void);
static int btmtk_usb_submit_bulk_in_urb(void);
static int btmtk_usb_send_hci_reset_cmd(void);
static int btmtk_usb_send_woble_suspend_cmd(void);
static void btmtk_usb_load_rom_patch_complete(const struct urb *urb);
static void btmtk_usb_bulk_in_complete(struct urb *urb);
static int btmtk_usb_submit_isoc_urb(void);
static void btmtk_usb_isoc_complete(struct urb *urb);
static void btmtk_usb_isoc_tx_complete(const struct urb *urb);
static void btmtk_usb_hci_snoop_print_to_log(void);
static int btmtk_usb_get_rom_patch_result(void);
static void btmtk_usb_tx_complete_meta(const struct urb *urb);
static int btmtk_usb_load_rom_patch(void);
static int btmtk_usb_send_wmt_reset_cmd(void);
static void btmtk_usb_intr_complete(struct urb *urb);
static int btmtk_usb_send_wmt_cmd(const u8 *cmd, const int cmd_len, const u8 *event,
		const int event_len, u32 delay, u32 retry_count);
static int btmtk_usb_send_hci_cmd(const u8 *cmd, int cmd_len, const u8 *event, int event_len);
static int btmtk_usb_add_to_hci_log(const u8 *buf, int buf_len, int hci_type);
static int btmtk_usb_push_data_to_metabuffer(u8 *buf, int buf_len, int hci_type);
/** file_operations: stpbt/stpbtfwlog */
static int btmtk_usb_fops_open(struct inode *inode, struct file *file);
static int btmtk_usb_fops_close(struct inode *inode, struct file *file);
static ssize_t btmtk_usb_fops_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t btmtk_usb_fops_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos);
static unsigned int btmtk_usb_fops_poll(struct file *file, poll_table *wait);
static long btmtk_usb_fops_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int btmtk_usb_fops_openfwlog(struct inode *inode, struct file *file);
static int btmtk_usb_fops_closefwlog(struct inode *inode, struct file *file);
static ssize_t btmtk_usb_fops_readfwlog(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t btmtk_usb_fops_writefwlog(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static unsigned int btmtk_usb_fops_pollfwlog(struct file *filp, poll_table *wait);
static long btmtk_usb_fops_unlocked_ioctlfwlog(struct file *filp, unsigned int cmd, unsigned long arg);

static int btmtk_usb_fops_sco_open(struct inode *inode, struct file *file);
static int btmtk_usb_fops_sco_close(struct inode *inode, struct file *file);
static ssize_t btmtk_usb_fops_sco_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t btmtk_usb_fops_sco_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos);
static unsigned int btmtk_usb_fops_sco_poll(struct file *file, poll_table *wait);
static long btmtk_usb_fops_sco_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/** read_write for proc */
static int btmtk_proc_show(struct seq_file *m, void *v);
static int btmtk_proc_open(struct inode *inode, struct  file *file);
static void btmtk_proc_create_new_entry(void);

#if SUPPORT_MT7662
static int btmtk_usb_load_rom_patch_7662(void);
static int btmtk_usb_io_read32_7662(u32 reg, u32 *val);
static int btmtk_usb_io_write32_7662(u32 reg, u32 val);
static int btmtk_usb_switch_iobase_7662(int base);
static int btmtk_usb_chk_crc_7662(u32 checksum_len);
static u16 btmtk_usb_get_crc_7662(void);
static u16 btmtk_usb_checksum16_7662(u8 *pData, int len);
static int btmtk_usb_send_hci_set_tx_power_cmd_7662(void);
static int btmtk_usb_send_hci_radio_on_cmd_7662(void);
static int btmtk_usb_send_hci_radio_off_cmd_7662(void);
static int btmtk_usb_send_hci_set_ce_cmd_7662(void);
static int btmtk_usb_send_hci_check_rom_patch_result_cmd_7662(void);
static int btmtk_usb_send_hci_low_power_cmd_7662(bool enable);
static void btmtk_usb_send_dummy_bulk_out_packet_7662(void);
#endif /* SUPPORT_MT7662 */

#if SUPPORT_MT7668 || SUPPORT_MT7663
static int btmtk_usb_load_rom_patch_7668(void);
static int btmtk_usb_io_read32_7668(u32 reg, u32 *val);
static int btmtk_usb_io_read32_7668(u32 reg, u32 *val);
static int btmtk_usb_send_hci_tci_set_sleep_cmd_7668(void);
static int btmtk_usb_send_wmt_power_on_cmd_7668(void);
static int btmtk_usb_send_wmt_power_off_cmd_7668(void);
static int btmtk_usb_reset_power_on(void);
#endif /* SUPPORT_MT7668 or SUPPORT_MT7663 */

static int btmtk_usb_unify_woble_wake_up(void);
static int btmtk_usb_unify_woble_suspend(struct btmtk_usb_data *data);
static int btmtk_usb_send_unify_woble_suspend_cmd(void);
static int btmtk_usb_send_leave_woble_suspend_cmd(void);
static int btmtk_usb_send_get_vendor_cap(void);

/*============================================================================*/
/* Global Variable */
/*============================================================================*/
#define _____________________________________________________Global_Variables
#define FWLOG_SPIN_LOCK(x)	spin_lock_irqsave(&g_data->fwlog_lock, x)
#define FWLOG_SPIN_UNLOCK(x)	spin_unlock_irqrestore(&g_data->fwlog_lock, x)
#define ISOC_SPIN_LOCK(x)	spin_lock_irqsave(&g_data->isoc_lock, x)
#define ISOC_SPIN_UNLOCK(x)	spin_unlock_irqrestore(&g_data->isoc_lock, x)

#define RETRY_TIMES 10

/* stpbt character device for meta */
#ifdef FIXED_STPBT_MAJOR_DEV_ID
static int BT_major = FIXED_STPBT_MAJOR_DEV_ID;
static int BT_majorfwlog = FIXED_STPBT_MAJOR_DEV_ID + 1;
static int BT_major_sco = FIXED_STPBT_MAJOR_DEV_ID + 2;
#else
static int BT_major;
static int BT_majorfwlog;
static int BT_major_sco;
#endif /* FIXED_STPBT_MAJOR_DEV_ID */

static struct class *pBTClass;
static struct device *pBTDev;
static struct device *pBTDevfwlog;
static struct device *pBTDev_sco;

static struct cdev BT_cdev;
static struct cdev BT_cdevfwlog;
static struct cdev BT_cdev_sco;

static struct usb_driver btmtk_usb_driver;
static wait_queue_head_t inq;
static wait_queue_head_t fw_log_inq;
static wait_queue_head_t inq_isoc;
static struct btmtk_usb_data *g_data;
static int probe_counter;
static u8 need_reset_stack;
static u8 need_reopen;
/* bluetooth KPI feautre, bperf */
static u8 btmtk_bluetooth_kpi;
static int leftHciEventSize;
static int leftACLSize;
static int btmtk_usb_state = BTMTK_USB_STATE_UNKNOWN;
static int btmtk_fops_state = BTMTK_FOPS_STATE_UNKNOWN;
static unsigned short sco_handle;	/* So far only support one SCO link */
static DECLARE_WAIT_QUEUE_HEAD(BT_wq);
static DECLARE_WAIT_QUEUE_HEAD(BT_sco_wq);
static DEFINE_MUTEX(btmtk_usb_state_mutex);
#define USB_MUTEX_LOCK()	mutex_lock(&btmtk_usb_state_mutex)
#define USB_MUTEX_UNLOCK()	mutex_unlock(&btmtk_usb_state_mutex)
static DEFINE_MUTEX(btmtk_fops_state_mutex);
#define FOPS_MUTEX_LOCK()	mutex_lock(&btmtk_fops_state_mutex)
#define FOPS_MUTEX_UNLOCK()	mutex_unlock(&btmtk_fops_state_mutex)

typedef void (*register_early_suspend) (void (*f) (void));
typedef void (*register_late_resume) (void (*f) (void));
register_early_suspend register_early_suspend_func;
register_late_resume register_late_resume_func;

/* Hci Snoop */
static u8 hci_cmd_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_cmd_snoop_len[HCI_SNOOP_ENTRY_NUM] = { 0 };

static unsigned int hci_cmd_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];
static u8 hci_event_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_event_snoop_len[HCI_SNOOP_ENTRY_NUM] = { 0 };

static unsigned int hci_event_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];
static u8 hci_acl_snoop_buf[HCI_SNOOP_ENTRY_NUM][HCI_SNOOP_BUF_SIZE];
static u8 hci_acl_snoop_len[HCI_SNOOP_ENTRY_NUM] = { 0 };

static unsigned int hci_acl_snoop_timestamp[HCI_SNOOP_ENTRY_NUM];
static int hci_cmd_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
static int hci_event_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
static int hci_acl_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;

static dev_t g_devIDfwlog;
static char fw_version_str[FW_VERSION_BUF_SIZE];
static struct proc_dir_entry *g_proc_dir;
u8 btmtk_log_lvl = BTMTK_LOG_LEVEL_DEFAULT;

/**
 * USB device ID configureation
 */
static struct usb_device_id btmtk_usb_table[] = {
#if SUPPORT_MT7662
	{USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7662, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0},	/* MT7662U */
	{USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7632, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0},	/* MT7632U */
	{USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x76a0, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0},	/* MT7662T */
	{USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x76a1, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0},	/* MT7632T */
#endif

#if SUPPORT_MT7668
	{USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7668, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0},
#endif

#if SUPPORT_MT7663
	{USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7663, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0},
#endif

	{}
};

const struct file_operations BT_proc_fops = {
	.open = btmtk_proc_open,
	.read = seq_read,
	.release = single_release,
};

const struct file_operations BT_fops = {
	.open = btmtk_usb_fops_open,
	.release = btmtk_usb_fops_close,
	.read = btmtk_usb_fops_read,
	.write = btmtk_usb_fops_write,
	.poll = btmtk_usb_fops_poll,
	.unlocked_ioctl = btmtk_usb_fops_unlocked_ioctl,
};

const struct file_operations BT_fopsfwlog = {
	.open = btmtk_usb_fops_openfwlog,
	.release = btmtk_usb_fops_closefwlog,
	.read = btmtk_usb_fops_readfwlog,
	.write = btmtk_usb_fops_writefwlog,
	.poll = btmtk_usb_fops_pollfwlog,
	.unlocked_ioctl = btmtk_usb_fops_unlocked_ioctlfwlog
};

const struct file_operations BT_sco_fops = {
	.open = btmtk_usb_fops_sco_open,
	.release = btmtk_usb_fops_sco_close,
	.read = btmtk_usb_fops_sco_read,
	.write = btmtk_usb_fops_sco_write,
	.poll = btmtk_usb_fops_sco_poll,
	.unlocked_ioctl = btmtk_usb_fops_sco_unlocked_ioctl,
};

static inline void __fill_isoc_descriptor(struct urb *urb, int len, int mtu)
{
	int i, offset = 0;

	BTUSB_DBG("%s: len %d mtu %d", __func__, len, mtu);
	for (i = 0; i < BTUSB_MAX_ISOC_FRAMES && len >= mtu;
			i++, offset += mtu, len -= mtu) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = mtu;
	}

	if (len && i < BTUSB_MAX_ISOC_FRAMES) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = len;
		i++;
	}

	urb->number_of_packets = i;
}

/*============================================================================*/
/* Internal Functions */
/*============================================================================*/
#define ___________________________________________________Internal_Functions
static int btmtk_skb_enq_fwlog(void *src, u32 len, u8 type, struct sk_buff_head *queue)
{
	struct sk_buff *skb_tmp = NULL;
	ulong flags = 0;
	int retry = RETRY_TIMES;

	do {
		/* If we need hci type, len + 1 */
		skb_tmp = alloc_skb(type ? len + 1 : len, GFP_ATOMIC);
		if (skb_tmp != NULL)
			break;
		else if (retry <= 0) {
			BTUSB_ERR("%s: alloc_skb return 0, error", __func__);
			return -ENOMEM;
		}
		BTUSB_ERR("%s: alloc_skb return 0, error, retry = %d", __func__, retry);
	} while (retry-- > 0);

	if (type) {
		memcpy(&skb_tmp->data[0], &type, 1);
		memcpy(&skb_tmp->data[1], src, len);
		skb_tmp->len = len + 1;
	} else {
		memcpy(skb_tmp->data, src, len);
		skb_tmp->len = len;
	}

	FWLOG_SPIN_LOCK(flags);
	skb_queue_tail(queue, skb_tmp);
	FWLOG_SPIN_UNLOCK(flags);
	return 0;
}

static int btmtk_usb_init_memory(void)
{
	if (g_data == NULL) {
		BTUSB_ERR("%s: g_data is NULL !", __func__);
		return -1;
	}

	g_data->chip_id = 0;
	g_data->suspend_count = 0;
	g_data->isoc_alt_setting = ISOC_IF_ALT_DEFAULT;

	if (g_data->metabuffer)
		memset(g_data->metabuffer->buffer, 0, META_BUFFER_SIZE);

	if (g_data->io_buf)
		memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);

	if (g_data->rom_patch_bin_file_name)
		memset(g_data->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);

	if (g_data->i_buf)
		memset(g_data->i_buf, 0, BUFFER_SIZE);

	if (g_data->o_buf)
		memset(g_data->o_buf, 0, BUFFER_SIZE);

	if (g_data->i_fwlog_buf)
		memset(g_data->i_fwlog_buf, 0, HCI_MAX_COMMAND_BUF_SIZE);

	if (g_data->o_fwlog_buf)
		memset(g_data->o_fwlog_buf, 0, HCI_MAX_COMMAND_SIZE);

	if (g_data->o_sco_buf)
		memset(g_data->o_sco_buf, 0, BUFFER_SIZE);

	if (g_data->o_usb_buf)
		memset(g_data->o_usb_buf, 0, HCI_MAX_COMMAND_SIZE);
	BTUSB_INFO("%s: Success", __func__);
	return 1;
}

static int btmtk_usb_allocate_memory(void)
{
	if (g_data == NULL) {
		g_data = kzalloc(sizeof(*g_data), GFP_KERNEL);
		if (!g_data) {
			BTUSB_ERR("%s: alloc memory fail (g_data)", __func__);
			return -1;
		}
	}

	if (g_data->metabuffer == NULL) {
		g_data->metabuffer = kzalloc(sizeof(struct ring_buffer_struct), GFP_KERNEL);
		if (!g_data->metabuffer) {
			BTUSB_ERR("%s: alloc memory fail (g_data->metabuffer)",
				__func__);
			return -1;
		}
	}

	if (g_data->io_buf == NULL) {
		g_data->io_buf = kzalloc(USB_IO_BUF_SIZE, GFP_KERNEL);
		if (!g_data->io_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->io_buf)", __func__);
			return -1;
		}
	}

	if (g_data->rom_patch_bin_file_name == NULL) {
		g_data->rom_patch_bin_file_name = kzalloc(MAX_BIN_FILE_NAME_LEN, GFP_KERNEL);
		if (!g_data->rom_patch_bin_file_name) {
			BTUSB_ERR("%s: alloc memory fail (g_data->rom_patch_bin_file_name)", __func__);
			return -1;
		}
	}

	if (g_data->woble_setting_file_name == NULL) {
		g_data->woble_setting_file_name = kzalloc(MAX_BIN_FILE_NAME_LEN, GFP_KERNEL);
		if (!g_data->woble_setting_file_name) {
			BTUSB_ERR("%s: alloc memory fail (g_data->woble_setting_file_name)", __func__);
			return -1;
		}
	}

	if (g_data->i_buf == NULL) {
		g_data->i_buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
		if (!g_data->i_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->i_buf)", __func__);
			return -1;
		}
	}

	if (g_data->o_buf == NULL) {
		g_data->o_buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
		if (!g_data->o_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->o_buf)", __func__);
			return -1;
		}
	}

#if ENABLE_BT_FIFO_THREAD
	if (g_data->bt_fifo == NULL) {
		g_data->bt_fifo = btmtk_fifo_init();
		if (!g_data->bt_fifo) {
			BTUSB_ERR("%s: alloc memory fail (g_data->bt_fifo)", __func__);
			return -1;
		}
	}
#endif

	if (g_data->i_fwlog_buf == NULL) {
		g_data->i_fwlog_buf = kzalloc(HCI_MAX_COMMAND_BUF_SIZE, GFP_KERNEL);
		if (!g_data->i_fwlog_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->i_fwlog_buf)", __func__);
			return -1;
		}
	}

	if (g_data->o_fwlog_buf == NULL) {
		g_data->o_fwlog_buf = kzalloc(HCI_MAX_COMMAND_SIZE, GFP_KERNEL);
		if (!g_data->o_fwlog_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->o_fwlog_buf)", __func__);
			return -1;
		}
	}

	if (g_data->o_sco_buf == NULL) {
		g_data->o_sco_buf = kzalloc(BUFFER_SIZE, GFP_KERNEL);
		if (g_data->o_sco_buf == NULL) {
			BTUSB_ERR("%s: alloc memory fail (g_data->o_sco_buf)",
				__func__);
			return -1;
		}
	}

	if (g_data->o_usb_buf == NULL) {
		g_data->o_usb_buf = kzalloc(HCI_MAX_COMMAND_SIZE, GFP_KERNEL);
		if (!g_data->o_usb_buf) {
			BTUSB_ERR("%s: alloc memory fail (g_data->o_usb_buf)", __func__);
			return -1;
		}
	}
	BTUSB_INFO("%s: Success", __func__);
	return 1;
}

static void btmtk_usb_free_memory(void)
{
	if (!g_data) {
		BTUSB_ERR("%s: g_data is NULL!", __func__);
		return;
	}

	kfree(g_data->metabuffer);
	g_data->metabuffer = NULL;

	kfree(g_data->i_buf);
	g_data->i_buf = NULL;

	kfree(g_data->o_buf);
	g_data->o_buf = NULL;

	kfree(g_data->i_fwlog_buf);
	g_data->i_fwlog_buf = NULL;

	kfree(g_data->o_fwlog_buf);
	g_data->o_fwlog_buf = NULL;

	kfree(g_data->o_sco_buf);
	g_data->o_sco_buf = NULL;

	kfree(g_data->rom_patch_bin_file_name);
	g_data->rom_patch_bin_file_name = NULL;

	kfree(g_data->woble_setting_file_name);
	g_data->woble_setting_file_name = NULL;

	kfree(g_data->io_buf);
	g_data->io_buf = NULL;

	kfree(g_data->rom_patch_image);
	g_data->rom_patch_image = NULL;

	kfree(g_data->o_usb_buf);
	g_data->o_usb_buf = NULL;

#if ENABLE_BT_FIFO_THREAD
	btmtk_fifo_init();
	g_data->bt_fifo = NULL;
#endif

	/* free queues */
	skb_queue_purge(&g_data->fwlog_queue);
	skb_queue_purge(&g_data->isoc_in_queue);

	kfree(g_data);
	g_data = NULL;

	BTUSB_INFO("%s: Success", __func__);
}

static int btmtk_usb_get_state(void)
{
	return btmtk_usb_state;
}

static void btmtk_usb_set_state(int new_state)
{
	static const char * const state_msg[] = {
		"UNKNOWN", "INIT", "DISCONNECT", "PROBE", "WORKING", "EARLY_SUSPEND",
		"SUSPEND", "RESUME", "LATE_RESUME", "FW_DUMP", "SUSPEND_DISCONNECT",
		"SUSPEND_PROBE", "SUSPEND_FW_DUMP", "RESUME_DISCONNECT", "RESUME_PROBE",
		"RESUME_FW_DUMP",
	};

	BTUSB_INFO("%s: %s(%d) -> %s(%d)", __func__, state_msg[btmtk_usb_state],
			btmtk_usb_state, state_msg[new_state], new_state);
	btmtk_usb_state = new_state;
}

static int btmtk_fops_get_state(void)
{
	return btmtk_fops_state;
}

static void btmtk_fops_set_state(int new_state)
{
	static const char * const fstate_msg[] = {"UNKNOWN", "INIT", "OPENED", "CLOSING", "CLOSED"};

	BTUSB_INFO("%s: FOPS_%s(%d) -> FOPS_%s(%d)", __func__, fstate_msg[btmtk_fops_state],
			btmtk_fops_state, fstate_msg[new_state], new_state);
	btmtk_fops_state = new_state;
}

static unsigned int btmtk_usb_hci_snoop_get_microseconds(void)
{
	struct timeval now;

	do_gettimeofday(&now);
	return now.tv_sec * 1000000 + now.tv_usec;
}

static int btmtk_usb_dispatch_data_bluetooth_kpi(u8 *buf, int len, u8 type)
{
	static u8 fwlog_blocking_warn;
	int ret = 0;

	if (btmtk_bluetooth_kpi &&
		skb_queue_len(&g_data->fwlog_queue) < FWLOG_BLUETOOTH_KPI_QUEUE_COUNT) {
		/* sent event to queue, picus tool will log it for bluetooth KPI feature */
		if (btmtk_skb_enq_fwlog(buf, len, type, &g_data->fwlog_queue) == 0) {
			wake_up_interruptible(&fw_log_inq);
			fwlog_blocking_warn = 0;
		}
	} else {
		if (fwlog_blocking_warn == 0) {
			fwlog_blocking_warn = 1;
			BTUSB_WARN("btmtk_usb fwlog queue size is full(bluetooth_kpi)");
		}
	}
	return ret;
}

static int btmtk_usb_dispatch_acl(u8 *buf, int len)
{
	static u8 fwlog_picus_blocking_warn;
	static u8 fwlog_fwdump_blocking_warn;
	int ret = 0;

	if (buf[0] == 0xff && buf[1] == 0x05) {
		/* picus or syslog */
		if (skb_queue_len(&g_data->fwlog_queue) < FWLOG_QUEUE_COUNT) {
			/* sent picus data to queue, picus tool will log it */
			if (btmtk_skb_enq_fwlog(buf, len, 0, &g_data->fwlog_queue) == 0) {
				wake_up_interruptible(&fw_log_inq);
				fwlog_picus_blocking_warn = 0;
			}
		} else {
			if (fwlog_picus_blocking_warn == 0) {
				fwlog_picus_blocking_warn = 1;
				BTUSB_WARN("btmtk_usb fwlog queue size is full(picus)");
			}
		}
	} else if (buf[0] == 0x6f && buf[1] == 0xfc) {
		/* Coredump */
		if (skb_queue_len(&g_data->fwlog_queue) < FWLOG_ASSERT_QUEUE_COUNT) {
			/* sent coredump data to queue, picus tool will log it */
			ret = btmtk_skb_enq_fwlog(buf, len, 0, &g_data->fwlog_queue);
			if (ret == 0)
				wake_up_interruptible(&fw_log_inq);
			else
				btmtk_usb_toggle_rst_pin();
			fwlog_fwdump_blocking_warn = 0;
		} else {
			if (fwlog_fwdump_blocking_warn == 0) {
				fwlog_fwdump_blocking_warn = 1;
				BTUSB_WARN("btmtk_usb fwlog queue size is full(coredump)");
			}
		}
	}
	return ret;
}

static int btmtk_usb_dispatch_event(u8 *buf, int len)
{
	static u8 fwlog_blocking_warn;
	int ret = 0;

	/* picus or syslog */
	if (buf[0] == 0xFF && buf[1] + 2 == len && (buf[2] == 0x50 || buf[2] == 0x51)) {
		if (skb_queue_len(&g_data->fwlog_queue) < FWLOG_QUEUE_COUNT) {
			/* sent coredump data to queue, picus will log it */
			if (btmtk_skb_enq_fwlog(buf, len, 0, &g_data->fwlog_queue) == 0) {
				wake_up_interruptible(&fw_log_inq);
				fwlog_blocking_warn = 0;
			}
		} else {
			if (fwlog_blocking_warn == 0) {
				fwlog_blocking_warn = 1;
				BTUSB_WARN("btmtk_usb fwlog queue size is full(picus)");
			}
		}

#define HCE_WAKEUP_DEBUG_EVENT 0xE7
	/* Wakeup debugging event */
	} else if (buf[0] == HCE_WAKEUP_DEBUG_EVENT && buf[1] == 0x10) {
		BTUSB_INFO("passSCAN:0x%02X, enterAPCF:0x%02X, passAPCF:0x%02X, toggleGPIO:0x%02X",
				*(u32 *)(buf + 2), *(u32 *)(buf + 6),
				*(u32 *)(buf + 10), *(u32 *)(buf + 14));
		btmtk_usb_add_to_hci_log(buf, len, HCI_EVENT_PKT);

	/* Unexpected event */
	} else {
		btmtk_usb_push_data_to_metabuffer(buf, len, HCI_EVENT_PKT);
		ret = -1;
	}
	return ret;
}

static void btmtk_usb_hci_snoop_init(void)
{
	int i;

	hci_cmd_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	hci_event_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	hci_acl_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	for (i = 0; i < HCI_SNOOP_ENTRY_NUM; i++) {
		hci_cmd_snoop_len[i] = 0;
		hci_event_snoop_len[i] = 0;
		hci_acl_snoop_len[i] = 0;
	}
}

static void btmtk_usb_hci_snoop_save_cmd(u32 len, u8 *buf)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;

	if (buf) {
		if (len < HCI_SNOOP_BUF_SIZE)
			copy_len = len;
		hci_cmd_snoop_len[hci_cmd_snoop_index] = copy_len & 0xff;
		memset(hci_cmd_snoop_buf[hci_cmd_snoop_index], 0, HCI_SNOOP_BUF_SIZE);
		memcpy(hci_cmd_snoop_buf[hci_cmd_snoop_index], buf, copy_len & 0xff);
		hci_cmd_snoop_timestamp[hci_cmd_snoop_index] = btmtk_usb_hci_snoop_get_microseconds();

		hci_cmd_snoop_index--;
		if (hci_cmd_snoop_index < 0)
			hci_cmd_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	}
}

static void btmtk_usb_hci_snoop_save_event(u32 len, u8 *buf)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;

	if (buf) {
		if (len < HCI_SNOOP_BUF_SIZE)
			copy_len = len;
		hci_event_snoop_len[hci_event_snoop_index] = copy_len;
		memset(hci_event_snoop_buf[hci_event_snoop_index], 0,
			HCI_SNOOP_BUF_SIZE);
		memcpy(hci_event_snoop_buf[hci_event_snoop_index], buf, copy_len);
		hci_event_snoop_timestamp[hci_event_snoop_index] = btmtk_usb_hci_snoop_get_microseconds();

		hci_event_snoop_index--;
		if (hci_event_snoop_index < 0)
			hci_event_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	}
}

static void btmtk_usb_hci_snoop_save_acl(u32 len, u8 *buf)
{
	u32 copy_len = HCI_SNOOP_BUF_SIZE;

	if (buf) {
		if (len < HCI_SNOOP_BUF_SIZE)
			copy_len = len;
		hci_acl_snoop_len[hci_acl_snoop_index] = copy_len & 0xff;
		memset(hci_acl_snoop_buf[hci_acl_snoop_index], 0, HCI_SNOOP_BUF_SIZE);
		memcpy(hci_acl_snoop_buf[hci_acl_snoop_index], buf, copy_len & 0xff);
		hci_acl_snoop_timestamp[hci_acl_snoop_index] = btmtk_usb_hci_snoop_get_microseconds();

		hci_acl_snoop_index--;
		if (hci_acl_snoop_index < 0)
			hci_acl_snoop_index = HCI_SNOOP_ENTRY_NUM - 1;
	}
}

static void btmtk_usb_hci_snoop_print_to_log(void)
{
	int counter, index, j;

	BTUSB_INFO("HCI Command Dump");
	BTUSB_INFO("  index(len)(timestamp:us) :HCI Command");
	index = hci_cmd_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_cmd_snoop_len[index] > 0) {
			pr_cont("	%d(%02d)(%u) :", counter,
							hci_cmd_snoop_len[index],
							hci_cmd_snoop_timestamp[index]);
			for (j = 0; j < hci_cmd_snoop_len[index]; j++)
				pr_cont("%02X ", hci_cmd_snoop_buf[index][j]);
			pr_cont("\n");
		}
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}

	BTUSB_INFO("HCI Event Dump");
	BTUSB_INFO("  index(len)(timestamp:us) :HCI Event");
	index = hci_event_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_event_snoop_len[index] > 0) {
			pr_cont("	%d(%02d)(%u) :", counter,
							hci_event_snoop_len[index],
							hci_event_snoop_timestamp[index]);
			for (j = 0; j < hci_event_snoop_len[index]; j++)
				pr_cont("%02X ", hci_event_snoop_buf[index][j]);
			pr_cont("\n");
		}
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}

	BTUSB_INFO("HCI ACL Dump");
	BTUSB_INFO("  index(len)(timestamp:us) :ACL");
	index = hci_acl_snoop_index + 1;
	if (index >= HCI_SNOOP_ENTRY_NUM)
		index = 0;
	for (counter = 0; counter < HCI_SNOOP_ENTRY_NUM; counter++) {
		if (hci_acl_snoop_len[index] > 0) {
			pr_cont("	%d(%02d)(%u) :", counter,
							hci_acl_snoop_len[index],
							hci_acl_snoop_timestamp[index]);
			for (j = 0; j < hci_acl_snoop_len[index]; j++)
				pr_cont("%02X ", hci_acl_snoop_buf[index][j]);
			pr_cont("\n");
		}
		index++;
		if (index >= HCI_SNOOP_ENTRY_NUM)
			index = 0;
	}
}

static int btmtk_usb_send_assert_cmd(void)
{
	int ret = 0;
	int state = btmtk_usb_get_state();

	if (state == BTMTK_USB_STATE_FW_DUMP || state == BTMTK_USB_STATE_SUSPEND_FW_DUMP
			|| state == BTMTK_USB_STATE_RESUME_FW_DUMP) {
		BTUSB_WARN("%s: FW dumping already!!!", __func__);
		return ret;
	}

	BTUSB_INFO("%s: send assert cmd", __func__);
	ret = btmtk_usb_send_assert_cmd_ctrl();
	if (ret < 0)
		ret = btmtk_usb_send_assert_cmd_bulk();
	if (ret < 0) {
		BTUSB_ERR("%s: send assert cmd fail, tigger hw reset only", __func__);
		btmtk_usb_toggle_rst_pin();
		return ret;
	}

	/* submit URB since btmtk_usb_send_hci_cmd would stop it */
	ret = btmtk_usb_start_traffic();
	if (ret < 0) {
		BTUSB_ERR("%s: Start traffic fail, tigger hw reset directly", __func__);
		btmtk_usb_toggle_rst_pin();
	}
	return ret;
}

void btmtk_usb_toggle_rst_pin(void)
{
	BTUSB_INFO("%s: begin", __func__);
	if (need_reset_stack == HW_ERR_NONE) {
		need_reset_stack = HW_ERR_CODE_CHIP_RESET;
		/* No Need print HCI records since trigger by WiFi */
	} else {
		btmtk_usb_hci_snoop_print_to_log();
	}

	do {
		typedef void (*pdwnc_func) (u8 fgReset);
		char *pdwnc_func_name = "PDWNC_SetBTInResetState";
		pdwnc_func pdwndFunc = (pdwnc_func) kallsyms_lookup_name(pdwnc_func_name);

		if (pdwndFunc) {
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, pdwnc_func_name, 1);
			pdwndFunc(1);
		} else
			BTUSB_INFO("%s: No Exported Func Found [%s]", __func__, pdwnc_func_name);
	} while (0);

	do {
		typedef void (*func_ptr) (int value);
		char *func_name = "setup_rstwlan_gpio";
		func_ptr pFunc = (func_ptr) kallsyms_lookup_name(func_name);

		if (pFunc) {
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, func_name, 0);
			pFunc(0);
			mdelay(RESET_PIN_SET_LOW_TIME);
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, func_name, 1);
			pFunc(1);
		} else {
			typedef void (*func_ptr) (unsigned int gpio, int init_value);
			char *func_name = "mtk_gpio_set_value";
			func_ptr pFunc = (func_ptr) kallsyms_lookup_name(func_name);

			if (pFunc) {
				BTUSB_INFO("%s: Invoke %s(%d,%d)", __func__, func_name, BT_DONGLE_RESET_GPIO_PIN, 0);
				pFunc(BT_DONGLE_RESET_GPIO_PIN, 0);
				mdelay(RESET_PIN_SET_LOW_TIME);
				BTUSB_INFO("%s: Invoke %s(%d,%d)", __func__, func_name, BT_DONGLE_RESET_GPIO_PIN, 1);
				pFunc(BT_DONGLE_RESET_GPIO_PIN, 1);
			} else
				BTUSB_INFO("%s: No Exported Func Found [%s]", __func__, func_name);
		}
	} while (0);

	do {
		typedef void (*set_low)(u8 gpio);
		typedef void (*set_high)(u8 gpio);
		char *set_low_name = "MDrv_GPIO_Set_Low";
		char *set_high_name = "MDrv_GPIO_Set_High";
		set_low pLow = (set_low)kallsyms_lookup_name(set_low_name);
		set_high pHigh = (set_high)kallsyms_lookup_name(set_high_name);

		if (pLow && pHigh) {
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, set_low_name, BT_DONGLE_RESET_GPIO_PIN);
			pLow(BT_DONGLE_RESET_GPIO_PIN);
			mdelay(RESET_PIN_SET_LOW_TIME);
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, set_high_name, BT_DONGLE_RESET_GPIO_PIN);
			pHigh(BT_DONGLE_RESET_GPIO_PIN);
		} else
			BTUSB_INFO("%s: No Exported Func Found [%s/%s]", __func__, set_low_name, set_high_name);

	} while (0);

	do {
		typedef void (*func_ptr) (int init_value);
		char *func_name = "extern_wifi_set_enable";
		func_ptr pFunc = (func_ptr) kallsyms_lookup_name(func_name);

		if (pFunc) {
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, func_name, 0);
			pFunc(0);
			mdelay(RESET_PIN_SET_LOW_TIME);
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, func_name, 1);
			pFunc(1);
		} else {
			BTUSB_INFO("%s: No Exported Func Found [%s]", __func__, func_name);
		}
	} while (0);
	BTUSB_INFO("%s: end", __func__);
}
EXPORT_SYMBOL(btmtk_usb_toggle_rst_pin);

static inline void btmtk_usb_lock_unsleepable_lock(struct OSAL_UNSLEEPABLE_LOCK *pUSL)
{
	spin_lock_irqsave(&(pUSL->lock), pUSL->flag);
}

static inline void btmtk_usb_unlock_unsleepable_lock(struct OSAL_UNSLEEPABLE_LOCK *pUSL)
{
	spin_unlock_irqrestore(&(pUSL->lock), pUSL->flag);
}

static void btmtk_usb_woble_free_setting_struct(struct woble_setting_struct *woble_struct, int count)
{
	int i = 0;

	for (i = 0; i < count; i++) {
		if (woble_struct[i].content) {
			BTUSB_INFO("%s:kfree %d", __func__, i);
			kfree(woble_struct[i].content);
			woble_struct[i].content = NULL;
			woble_struct[i].length = 0;
		} else
			woble_struct[i].length = 0;
	}
}

static void btmtk_usb_woble_free_setting(void)
{
	BTUSB_INFO("%s begin", __func__);
	if (g_data == NULL) {
		BTUSB_ERR("%s: g_data == NULL", __func__);
		return;
	}

	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_apcf, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_apcf_fill_mac, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_apcf_fill_mac_location, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(g_data->woble_setting_apcf_resume, WOBLE_SETTING_COUNT);
	btmtk_usb_woble_free_setting_struct(&g_data->woble_setting_radio_off, 1);
	btmtk_usb_woble_free_setting_struct(&g_data->woble_setting_radio_off_status_event, 1);
	btmtk_usb_woble_free_setting_struct(&g_data->woble_setting_radio_off_comp_event, 1);
	btmtk_usb_woble_free_setting_struct(&g_data->woble_setting_radio_on, 1);
	btmtk_usb_woble_free_setting_struct(&g_data->woble_setting_radio_on_status_event, 1);
	btmtk_usb_woble_free_setting_struct(&g_data->woble_setting_radio_on_comp_event, 1);

	g_data->woble_setting_len = 0;
}

static int btmtk_usb_load_woble_block_setting(char *block_name, struct woble_setting_struct *save_content,
		int counter, u8 *searchcontent)
{
	int ret = 0, i = 0;
	int temp_len = 0;
	u8 temp[128]; /* save for total hex number */
	unsigned long parsing_result = 0;
	char *search_result = NULL;
	char *search_end = NULL;
	char search[32];
	char *next_block = NULL;
#define CHAR2HEX_SIZE	4
	char number[CHAR2HEX_SIZE + 1];	/* 1 is for '\0' */

	memset(search, 0, sizeof(search));
	memset(temp, 0, sizeof(temp));
	memset(number, 0, sizeof(number));

	/* search block name */
	for (i = 0; i < counter; i++) {
		temp_len = 0;
		snprintf(search, sizeof(search), "%s%02d:", block_name, i); /* EX: APCF01 */
		search_result = strstr((char *)searchcontent, search);

		if (search_result) {
			memset(temp, 0, sizeof(temp));
			search_result = strstr(search_result, "0x");
			/* find next line as end of this command line, if NULL means last line */
			next_block = strstr(search_result, ":");

			do {
				search_end = strstr(search_result, ",");
				if (search_end - search_result != CHAR2HEX_SIZE) {
					BTUSB_ERR("%s: Incorrect Format in %s", __func__, search);
					break;
				}

				memset(number, 0, sizeof(number));
				memcpy(number, search_result, CHAR2HEX_SIZE);
				ret = kstrtoul(number, 0, &parsing_result);
				if (ret == 0) {
					if (temp_len >= sizeof(temp)) {
						BTUSB_ERR("%s: %s data over %zu", __func__, search, sizeof(temp));
						break;
					}
					temp[temp_len++] = parsing_result;
				} else {
					BTUSB_WARN("%s: %s kstrtoul fail: %d", __func__, search, ret);
					break;
				}
				search_result = strstr(search_end, "0x");
			} while (search_result < next_block || (search_result && next_block == NULL));
		} else
			BTUSB_DBG("%s: %s is not found in %d", __func__, search, i);

		if (temp_len && temp_len < sizeof(temp)) {
			BTUSB_INFO("%s: %s found & stored in %d", __func__, search, i);
			save_content[i].content = kzalloc(temp_len, GFP_KERNEL);
			if (save_content[i].content == NULL) {
				BTUSB_ERR("%s: Allocate memory fail(%d)", __func__, i);
				return -ENOMEM;
			}
			memcpy(save_content[i].content, temp, temp_len);
			save_content[i].length = temp_len;
			BTUSB_DBG_RAW(save_content[i].content, save_content[i].length, "%s", search);
		}
	}
	return ret;
}

static int btmtk_usb_load_woble_setting(char *bin_name,
		struct device *dev, u32 *code_len, struct btmtk_usb_data *data)
{
	int err;
	const struct firmware *fw_entry = NULL;
	u8 *image;

	*code_len = 0;

	BTUSB_INFO("%s: begin woble_setting_file_name = %s", __func__, bin_name);
	err = request_firmware(&fw_entry, bin_name, dev);
	if (err != 0 || fw_entry == NULL) {
		BTUSB_ERR("%s: request_firmware function fail!! error code = %d, fw_entry = %p",
				__func__, err, fw_entry);
		if (fw_entry)
			release_firmware(fw_entry);
		return err;
	}

	BTUSB_INFO("%s: woble_setting request_firmware size %zu success", __func__, fw_entry->size);
	image = kzalloc(fw_entry->size + 1, GFP_KERNEL); /* w:move to btmtk_usb_free_memory */
	if (image == NULL) {
		BTUSB_ERR("%s: kzalloc size %zu failed!!", __func__, fw_entry->size);
		release_firmware(fw_entry);
		return err;
	}

	memcpy(image, fw_entry->data, fw_entry->size);
	image[fw_entry->size] = '\0';

	*code_len = fw_entry->size;
	BTUSB_INFO("%s: code_len (%d) assign done", __func__, *code_len);

	err = btmtk_usb_load_woble_block_setting("APCF",
			data->woble_setting_apcf, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("APCF_ADD_MAC",
			data->woble_setting_apcf_fill_mac, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("APCF_ADD_MAC_LOCATION",
			data->woble_setting_apcf_fill_mac_location, WOBLE_SETTING_COUNT, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOOFF", &data->woble_setting_radio_off, 1, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOOFF_STATUS_EVENT",
			&data->woble_setting_radio_off_status_event, 1, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOOFF_COMPLETE_EVENT",
			&data->woble_setting_radio_off_comp_event, 1, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOON",
			&data->woble_setting_radio_on, 1, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOON_STATUS_EVENT",
			&data->woble_setting_radio_on_status_event, 1, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("RADIOON_COMPLETE_EVENT",
			&data->woble_setting_radio_on_comp_event, 1, image);
	if (err)
		goto LOAD_END;

	err = btmtk_usb_load_woble_block_setting("APCF_RESMUE",
			data->woble_setting_apcf_resume, WOBLE_SETTING_COUNT, image);

LOAD_END:
	kfree(image);
	release_firmware(fw_entry);
	if (err)
		BTUSB_INFO("%s: error return %d", __func__, err);

	return err;
}

static void btmtk_usb_load_code_from_bin(u8 **image, char *bin_name,
					 struct device *dev, u32 *code_len)
{
	const struct firmware *fw_entry;
	int err = 0;
	static u32 chip_id;
	int retry = RETRY_TIMES;

	if (g_data->rom_patch_image && g_data->rom_patch_image_len && g_data->chip_id == chip_id) {
		/* no need to request firmware again. */
		*image = g_data->rom_patch_image;
		*code_len = g_data->rom_patch_image_len;
		return;

	} else {
		chip_id = g_data->chip_id;
		kfree(g_data->rom_patch_image);
		g_data->rom_patch_image = NULL;
		g_data->rom_patch_image_len = 0;
	}

	do {
		err = request_firmware(&fw_entry, bin_name, dev);
		if (err == 0) {
			break;
		} else if (retry <= 0) {
			*image = NULL;
			BTUSB_ERR("%s: request_firmware %d times fail!!! err = %d", __func__, RETRY_TIMES, err);
			return;
		}
		BTUSB_ERR("%s: request_firmware fail!!! err = %d, retry = %d", __func__, err, retry);
		msleep(100);
	} while (retry-- > 0);

	*image = kzalloc(fw_entry->size, GFP_KERNEL);
	if (*image == NULL) {
		BTUSB_ERR("%s: kzalloc failed!! error code = %d", __func__, err);
		return;
	}

	memcpy(*image, fw_entry->data, fw_entry->size);
	*code_len = fw_entry->size;

	g_data->rom_patch_image = *image;
	g_data->rom_patch_image_len = *code_len;

	release_firmware(fw_entry);
}

static int btmtk_usb_start_traffic(void)
{
	int intr = 0, bulk = 0;

	BTUSB_INFO("%s", __func__);
	if (g_data->interrupt_urb_submitted == 0)
		intr = btmtk_usb_submit_intr_urb();
	if (g_data->bulk_urb_submitted == 0)
		bulk = btmtk_usb_submit_bulk_in_urb();

	return (intr | bulk);
}

static void btmtk_usb_stop_traffic(void)
{
	if ((g_data->bulk_urb_submitted == 0) && (g_data->interrupt_urb_submitted == 0)
			&& (g_data->isoc_urb_submitted == 0))
		return;

	BTUSB_INFO("%s", __func__);
	usb_kill_anchored_urbs(&g_data->intr_in_anchor);
	usb_kill_anchored_urbs(&g_data->bulk_in_anchor);
	usb_kill_anchored_urbs(&g_data->isoc_in_anchor);

	g_data->bulk_urb_submitted = 0;
	g_data->interrupt_urb_submitted = 0;
	g_data->isoc_urb_submitted = 0;
}

static void btmtk_usb_waker(struct work_struct *work)
{
	int err;

	err = usb_autopm_get_interface(g_data->intf);
	if (err < 0)
		return;

	usb_autopm_put_interface(g_data->intf);
}

static int btmtk_usb_submit_intr_urb(void)
{
	struct urb *urb;
	u8 *buf;
	unsigned int pipe;
	int err, size;

	BTUSB_INFO("%s: begin", __func__);

	if (g_data->interrupt_urb_submitted) {
		BTUSB_WARN("%s: already submitted", __func__);
		return 0;
	}
	g_data->interrupt_urb_submitted = 0;

	if (!g_data->intr_ep) {
		BTUSB_ERR("%s: error 1", __func__);
		return -ENODEV;
	}

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		BTUSB_ERR("%s: error 2", __func__);
		return -ENOMEM;
	}
	/* size = le16_to_cpu(g_data->intr_ep->wMaxPacketSize); */
	size = le16_to_cpu(HCI_MAX_EVENT_SIZE);
	BTUSB_INFO("%s: maximum packet size:%d", __func__, size);

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf) {
		usb_free_urb(urb);
		BTUSB_ERR("%s: error 3", __func__);
		return -ENOMEM;
	}

	pipe = usb_rcvintpipe(g_data->udev, g_data->intr_ep->bEndpointAddress);

	usb_fill_int_urb(urb, g_data->udev, pipe, buf, size,
			 (usb_complete_t)btmtk_usb_intr_complete, (void *)g_data,
			 g_data->intr_ep->bInterval);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &g_data->intr_in_anchor);

	err = usb_submit_urb(urb, GFP_KERNEL);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s: urb %p submission failed (%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	} else {
		g_data->interrupt_urb_submitted = 1;
	}

	usb_free_urb(urb);
	BTUSB_INFO("%s: end", __func__);
	return err;
}

static int btmtk_usb_submit_bulk_in_urb(void)
{
	struct urb *urb = NULL;
	u8 *buf = NULL;
	unsigned int pipe = 0;
	int err = 0;
	int size = HCI_MAX_FRAME_SIZE;

	BTUSB_INFO("%s: begin", __func__);

	if (g_data->bulk_urb_submitted) {
		BTUSB_WARN("%s: already submitted", __func__);
		return 0;
	}
	g_data->bulk_urb_submitted = 0;

	if (!g_data->bulk_rx_ep) {
		BTUSB_ERR("%s: end error 1", __func__);
		return -ENODEV;
	}

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		BTUSB_ERR("%s: end error 2", __func__);
		return -ENOMEM;
	}

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf) {
		usb_free_urb(urb);
		BTUSB_ERR("%s: end error 3", __func__);
		return -ENOMEM;
	}
	pipe = usb_rcvbulkpipe(g_data->udev, g_data->bulk_rx_ep->bEndpointAddress);
	usb_fill_bulk_urb(urb, g_data->udev, pipe, buf, size,
				btmtk_usb_bulk_in_complete, g_data);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_mark_last_busy(g_data->udev);
	usb_anchor_urb(urb, &g_data->bulk_in_anchor);

	err = usb_submit_urb(urb, GFP_KERNEL);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s: urb %p submission failed (%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	} else {
		g_data->bulk_urb_submitted = 1;
	}

	usb_free_urb(urb);
	BTUSB_INFO("%s: end", __func__);
	return err;
}

static int btmtk_usb_send_wmt_cmd(const u8 *cmd, const int cmd_len,
		const u8 *event, const int event_len, u32 delay, u32 retry_count)
{
	int ret = -1;	/* if successful, read length */
	int i = 0;
	bool check = FALSE;

	if (g_data == NULL || g_data->udev == NULL || g_data->io_buf == NULL ||
		g_data->o_usb_buf == NULL || cmd == NULL ||
		cmd_len > HCI_MAX_COMMAND_SIZE || cmd_len <= 0) {
		BTUSB_ERR("%s: incorrect cmd pointer", __func__);
		return ret;
	}
	if (event != NULL && event_len > 0)
		check = TRUE;

	/* send WMT command */
	memcpy(g_data->o_usb_buf, cmd, cmd_len);
	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0),
				0x01, DEVICE_CLASS_REQUEST_OUT, 0x30, 0x00, (void *)g_data->o_usb_buf, cmd_len,
				USB_CTRL_IO_TIMO);
	if (ret < 0) {
		BTUSB_ERR("%s: command send failed(%d)", __func__, ret);
		return ret;
	}

get_response_again:
	/* ms delay */
	mdelay(delay);

	/* check WMT event */
	memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
	ret = usb_control_msg(g_data->udev, usb_rcvctrlpipe(g_data->udev, 0),
				0x01, DEVICE_VENDOR_REQUEST_IN, 0x30, 0x00, g_data->io_buf,
				USB_IO_BUF_SIZE, USB_CTRL_IO_TIMO);
	if (ret < 0) {
		BTUSB_ERR("%s: event get failed(%d)", __func__, ret);
		if (check == TRUE)
			return ret;
		else
			return 0;	/* Do not ask read so return 0 */
	}

	if (check == TRUE) {
		if (ret >= event_len && !memcmp(event, g_data->io_buf, event_len)) {
			return ret; /* return read length */
		} else if (retry_count > 0) {
			BTUSB_WARN("%s: Trying to get response... (%d)", __func__, ret);
			retry_count--;
			goto get_response_again;
		} else {
			BTUSB_ERR("%s: got unknown event:(%d)", __func__, event_len);
			pr_cont("\t");
			for (i = 0; i < ret && i < 64; i++)
				pr_cont("%02X ", g_data->io_buf[i]);
			pr_cont("\n");
		}
	}
	return -1;
}

static int btmtk_usb_push_data_to_metabuffer(u8 *buf, int buf_len, int hci_type)
{
	int ret = -1;
	u32 roomLeft = 0, last_len = 0, length = buf_len;

	BTUSB_INFO("%s", __func__);
	if (!g_data) {
		BTUSB_ERR("%s: g_data==NULL return", __func__);
		return ret;
	}

	btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	/* roomleft means the usable space */
	if (g_data->metabuffer->read_p <= g_data->metabuffer->write_p)
		roomLeft = META_BUFFER_SIZE - g_data->metabuffer->write_p +
				g_data->metabuffer->read_p - 1;
	else
		roomLeft = g_data->metabuffer->read_p - g_data->metabuffer->write_p - 1;

	/* no enough space to store the received data */
	if (roomLeft < length)
		BTUSB_WARN("%s: Queue is full !!", __func__);

	if (length + g_data->metabuffer->write_p < META_BUFFER_SIZE) {
		if (leftACLSize == 0) {
			/* copy ACL data header: 0x02 */
			g_data->metabuffer->buffer[g_data->metabuffer->write_p] = hci_type;
			g_data->metabuffer->write_p += 1;
		}

		/* copy event data */
		memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
				buf, buf_len);
		g_data->metabuffer->write_p += buf_len;
	} else {
		last_len = META_BUFFER_SIZE - g_data->metabuffer->write_p;
		if (leftACLSize == 0) {
			if (last_len != 0) {
				/* copy ACL data header: 0x02 */
				if (g_data->metabuffer->write_p < META_BUFFER_SIZE)
					g_data->metabuffer->buffer[g_data->metabuffer->write_p] = hci_type;
				else {
					btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
					BTUSB_ERR("%s: write_p = %d  META_BUFFER_SIZE(%d)",
						__func__, g_data->metabuffer->write_p, META_BUFFER_SIZE);
					return ret;
				}

				g_data->metabuffer->write_p += 1;
				last_len--;
				/* copy event data */
				if (g_data->metabuffer->write_p != META_BUFFER_SIZE)
					memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
							buf, last_len);
				memcpy(g_data->metabuffer->buffer, buf + last_len,
						buf_len - last_len);
				g_data->metabuffer->write_p = buf_len - last_len;
			} else {
				g_data->metabuffer->buffer[0] = hci_type;
				g_data->metabuffer->write_p = 1;
				/* copy event data */
				memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
						buf, buf_len);
				g_data->metabuffer->write_p += buf_len;
			}
		} else {	/* leftACLSize !=0 */
			/* copy event data */
			BTUSB_WARN("%s: leftACLSize = %d", __func__, leftACLSize);
			memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
					buf, last_len);
			memcpy(g_data->metabuffer->buffer, buf + last_len,
					buf_len - last_len);
			g_data->metabuffer->write_p = buf_len - last_len;
		}
	}
	btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	return ret;
}

static int btmtk_usb_add_to_hci_log(const u8 *buf, int buf_len, int hci_type)
{
	u8 *alloc_buf = NULL;
	int ret = -1;
	u8 header[] = { 0xff, 0x00, 0xFE, 0x00 };
	int header_len = 0;

	header_len = sizeof(header);
	if (buf_len <= 0) {
		BTUSB_ERR("%s: buf_len = %d error", __func__, buf_len);
		return ret;
	}
	header[1] = buf_len + 2; /* 2 is include 0xfe, 0x00(hci type) */
	header[3] = hci_type;
	alloc_buf = kzalloc(buf_len + header_len, GFP_KERNEL);
	if (alloc_buf == NULL)
		return ret;

	memcpy(alloc_buf, header, header_len);
	memcpy(alloc_buf + header_len, buf, buf_len);
	ret = btmtk_usb_push_data_to_metabuffer(alloc_buf, buf_len + header_len, HCI_EVENT_PKT);
	kfree(alloc_buf);
	return ret;
}

static int btmtk_usb_send_hci_cmd(const u8 *cmd, const int cmd_len,
		const u8 *event, const int event_len)
{
	/** return length of even if compare successfully., 0 no need check event., < 0 error */
	int ret = -1;	/* if successful, read length */
	int len = 0;
	int i = 0;
	unsigned long timo = 0;
	bool check = FALSE;

	if (is_mt7668(g_data) || is_mt7663(g_data)) {
		if (g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_POWER_ON &&
				g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_WOBLE) {
			BTUSB_WARN("%s: chip power isn't on, ignore this command, state is %d", __func__,
					g_data->is_mt7668_dongle_state);
			return ret;
		}
	}

	/* parameter check */
	if (g_data == NULL || g_data->udev == NULL || g_data->io_buf == NULL ||
		g_data->o_usb_buf == NULL || cmd == NULL ||
		cmd_len > HCI_MAX_COMMAND_SIZE || cmd_len <= 0) {
		BTUSB_ERR("%s: incorrect cmd pointer", __func__);
		return ret;
	}
	if (event != NULL && event_len > 0)
		check = TRUE;

	/* need get event by interrupt, stop traffic before cmd send */
	btmtk_usb_stop_traffic();

	/* send HCI command */
	memcpy(g_data->o_usb_buf, cmd, cmd_len);
	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0),
				0, DEVICE_CLASS_REQUEST_OUT, 0, 0,
				(void *)g_data->o_usb_buf, cmd_len, USB_CTRL_IO_TIMO);
	if (ret < 0) {
		BTUSB_ERR("%s: command send failed(%d)", __func__, ret);
		return ret;
	}

	if (event_len == -1) {
		/**
		 * If event_len == -1, DO NOT read event, since FW wouldn't feedback
		 */
		return 0;
	}

	/* check HCI event */
	timo = jiffies + msecs_to_jiffies(USB_INTR_MSG_TIMO);
	do {
		memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
		ret = usb_interrupt_msg(g_data->udev, usb_rcvintpipe(g_data->udev, 1),
					g_data->io_buf, USB_IO_BUF_SIZE, &len, USB_INTR_MSG_TIMO);
		if (ret < 0) {
			BTUSB_ERR("%s: event get failed(%d)", __func__, ret);
			if (check == TRUE)
				return ret;
			else
				return 0;	/* Do not ask read so return 0 */
		}

		if (len >= 2 && g_data->io_buf[1] + 2 != len) {
			/* Incorrect packet format and length */
			BTUSB_WARN("%s: Ignore incorrect format packet:%02X %02X",
					__func__, g_data->io_buf[0], g_data->io_buf[1]);
			continue;
		}

		if (check == TRUE) {
			/* maybe don't care some returned parameters */
			if (len >= event_len) {
				for (i = 0; i < event_len; i++) {
					if (event[i] != g_data->io_buf[i])
						break;
				}
			} else {
				BTUSB_ERR("%s: event length is not match(%d/%d)", __func__, len, event_len);
			}
			if (i != event_len) {
				/* In case standard or picus event */
				if (btmtk_usb_dispatch_event(g_data->io_buf, event_len) < 0) {
					BTUSB_WARN("%s: got unexpected event:(%d/%d)", __func__, i, event_len);
					pr_cont("\t");
					for (i = 0; i < len && i < 64; i++)
						pr_cont("%02X ", g_data->io_buf[i]);
					pr_cont("\n");
				}
			} else
				return len; /* actually read length */
		}
	} while (time_before(jiffies, timo));

	BTUSB_ERR("%s: error, got event timeout, jiffies = %lu", __func__, jiffies);
	return -1;
}

int btmtk_usb_meta_send_data(const u8 *buffer, const unsigned int length)
{
	int ret = 0;

	if (buffer[0] != HCI_COMMAND_PKT) {
		BTUSB_WARN("the data from meta isn't HCI command, value: 0x%X", buffer[0]);
	} else {
		u8 *buf = (u8 *)buffer;

		ret = usb_control_msg(g_data->udev,
					usb_sndctrlpipe(g_data->udev, 0), 0x0,
					DEVICE_CLASS_REQUEST_OUT, 0x00, 0x00,
					buf + 1, length - 1,
					USB_CTRL_IO_TIMO);
	}

	if (ret < 0) {
		BTUSB_ERR("%s: error1(%d)", __func__, ret);
		return ret;
	}

	return length;
}

int btmtk_usb_send_data(const u8 *buffer, const unsigned int length)
{
	struct urb *urb = NULL;
	unsigned int pipe;
	int err;
	int send_data_len = length - 1;
	char *buf = NULL;

	if (!g_data->bulk_tx_ep) {
		BTUSB_ERR("%s: No bulk_tx_ep", __func__);
		return -ENODEV;
	}

	if (buffer[0] == HCI_ACLDATA_PKT) {
		while (g_data->meta_tx != 0)
			msleep(1);

		g_data->meta_tx = 1;
		urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!urb) {
			BTUSB_ERR("%s: No memory for ACL", __func__);
			return -ENOMEM;
		}
		buf = usb_alloc_coherent(g_data->udev, send_data_len, GFP_KERNEL, &urb->transfer_dma);

		urb->transfer_buffer = buf;
		urb->transfer_buffer_length = send_data_len;

		if (!buf) {
			BTUSB_ERR("%s: usb_alloc_coherent error", __func__);
			err = -ENOMEM;
			goto error_buffer;
		}
		/* remove the ACL header:0x02 */
		memcpy(buf, buffer + 1, send_data_len);

		pipe = usb_sndbulkpipe(g_data->udev, g_data->bulk_tx_ep->bEndpointAddress);

		usb_fill_bulk_urb(urb, g_data->udev, pipe, buf,
					length - 1, (usb_complete_t)btmtk_usb_tx_complete_meta,
					(void *)g_data);

		urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

		usb_anchor_urb(urb, &g_data->bulk_out_anchor);

		err = usb_submit_urb(urb, GFP_KERNEL);
		if (err == 0)
			err = length;
		else
			BTUSB_ERR("send ACL data fail, error:%d", err);
		mdelay(1);

	} else if (buffer[0] == HCI_SCODATA_PKT) {
		BTUSB_DBG("%s: HCI_SCODATA_PKT start (%d/%d)", __func__,
				send_data_len, g_data->isoc_tx_ep->wMaxPacketSize);
		if (!g_data->isoc_tx_ep) {
			BTUSB_ERR("%s: No isoc_tx_ep", __func__);
			return -ENODEV;
		}
		urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, GFP_KERNEL);
		if (!urb) {
			BTUSB_ERR("%s: No memory for SCO", __func__);
			return -ENOMEM;
		}
		pipe = usb_sndisocpipe(g_data->udev, g_data->isoc_tx_ep->bEndpointAddress);
		buf = kzalloc(send_data_len, GFP_KERNEL);
		memcpy(buf, buffer + 1, send_data_len);
		usb_fill_int_urb(urb, g_data->udev, pipe, buf,
			send_data_len, (usb_complete_t)btmtk_usb_isoc_tx_complete,
			(void *)g_data, g_data->isoc_tx_ep->bInterval);

		urb->transfer_flags = URB_ISO_ASAP;

		__fill_isoc_descriptor(urb, send_data_len,
			le16_to_cpu(g_data->isoc_tx_ep->wMaxPacketSize));
		usb_anchor_urb(urb, &g_data->isoc_out_anchor);

		atomic_inc(&g_data->isoc_out_count);
		err = usb_submit_urb(urb, GFP_KERNEL);
		if (err == 0) {
			err = length;
			BTUSB_DBG("%s: HCI_SCODATA_PKT end", __func__);
		} else {
			BTUSB_ERR("submit SCO urb fail, error:%d", err);
			err = -1;
		}
	} else {
		BTUSB_WARN("%s: unknown data", __func__);
		err = -1;
	}

error_buffer:
	if (err < 0) {
		if (urb) {
			if (buffer[0] == HCI_ACLDATA_PKT)
				usb_free_coherent(g_data->udev, urb->transfer_buffer_length,
						urb->transfer_buffer, urb->transfer_dma);
			else if (buffer[0] == HCI_SCODATA_PKT)
				kfree(urb->transfer_buffer);
			kfree(urb->setup_packet);
			usb_unanchor_urb(urb);
		}
	} else {
		usb_mark_last_busy(g_data->udev);
	}

	usb_free_urb(urb);
	return err;
}

static inline void btmtk_usb_woble_wake_lock(struct btmtk_usb_data *data)
{
#if (SUPPORT_UNIFY_WOBLE & SUPPORT_ANDROID)
	if (wake_lock_active(&data->woble_wlock) == 0) {
		BTUSB_INFO("%s: WoBLE WakeLock Enable", __func__);
		wake_lock(&data->woble_wlock);
	}
#endif
}

static inline void btmtk_usb_woble_wake_unlock(struct btmtk_usb_data *data)
{
#if (SUPPORT_UNIFY_WOBLE & SUPPORT_ANDROID)
	if (wake_lock_active(&data->woble_wlock) > 0) {
		BTUSB_INFO("%s: WoBLE WakeLock Disable", __func__);
		wake_unlock(&data->woble_wlock);
	}
#endif
}

static int btmtk_usb_BT_init(void)
{
	dev_t devID = MKDEV(BT_major, 0);
	dev_t devIDfwlog = MKDEV(BT_majorfwlog, 1);
	dev_t devID_sco = MKDEV(BT_major_sco, 0);
	int ret0 = -1, ret1 = -1, ret2 = -1;
	int cdevErr0 = 0, cdevErr1 = 0, cdevErr2 = 0;
	int major = 0;
	int majorfwlog = 0;
	int major_sco = 0;

	BTUSB_INFO("%s: devID %d, devID_fwlog %d, devID_sco %d", __func__, devID, devIDfwlog, devID_sco);

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_INIT);
	USB_MUTEX_UNLOCK();

	btmtk_proc_create_new_entry();

#ifdef FIXED_STPBT_MAJOR_DEV_ID
	ret0 = register_chrdev_region(devID, 1, "BT_chrdev");
	if (ret0) {
		BTUSB_ERR("fail to register_chrdev %d, allocate chrdev again", MAJOR(devID));
		ret0 = alloc_chrdev_region(&devID, 0, 1, "BT_chrdev");
		if (ret0) {
			BTUSB_ERR("fail to allocate chrdev");
			return ret0;
		}
	}

	ret1 = register_chrdev_region(devIDfwlog, 1, "BT_chrdevfwlog");
	if (ret1) {
		BTUSB_ERR("fail to register_chrdev %d, allocate chrdev again", MAJOR(devIDfwlog));
		ret1 = alloc_chrdev_region(&devIDfwlog, 0, 1, "BT_chrdevfwlog");
		if (ret1) {
			BTUSB_ERR("fail to allocate chrdev fwlog");
			goto err0;
		}
	}

	ret2 = register_chrdev_region(devID_sco, 1, "BT_chrdev_sco");
	if (ret2) {
		BTUSB_ERR("fail to register_chrdev %d, allocate chrdev again", MAJOR(devID_sco));
		ret2 = alloc_chrdev_region(&devID_sco, 0, 1, "BT_chrdev_sco");
		if (ret2) {
			BTUSB_ERR("fail to allocate chrdev sco");
			goto err0;
		}
	}
#else /* FIXED_STPBT_MAJOR_DEV_ID */
	ret0 = alloc_chrdev_region(&devID, 0, 1, "BT_chrdev");
	if (ret0) {
		BTUSB_ERR("fail to allocate chrdev");
		return ret0;
	}

	ret1 = alloc_chrdev_region(&devIDfwlog, 0, 1, "BT_chrdevfwlog");
	if (ret1) {
		BTUSB_ERR("fail to allocate chrdev");
		goto err0;
	}
	ret2 = alloc_chrdev_region(&devID_sco, 0, 1, "BT_chrdev_sco");
	if (ret2) {
		BTUSB_ERR("fail to allocate chrdev sco");
		goto err0;
	}
#endif /* FIXED_STPBT_MAJOR_DEV_ID */

	BT_major = major = MAJOR(devID);
	BTUSB_INFO("%s: major number: %d", __func__, BT_major);
	BT_majorfwlog = majorfwlog = MAJOR(devIDfwlog);
	BTUSB_INFO("%s: BT_majorfwlog number: %d", __func__, BT_majorfwlog);
	BT_major_sco = major_sco = MAJOR(devID_sco);
	BTUSB_INFO("%s: BT_major_sco number: %d", __func__, BT_major_sco);

	cdev_init(&BT_cdev, &BT_fops);
	BT_cdev.owner = THIS_MODULE;
	cdev_init(&BT_cdev_sco, &BT_sco_fops);
	BT_cdev_sco.owner = THIS_MODULE;

	cdev_init(&BT_cdevfwlog, &BT_fopsfwlog);
	BT_cdevfwlog.owner = THIS_MODULE;
	cdevErr0 = cdev_add(&BT_cdev, devID, 1);
	if (cdevErr0)
		goto err1;
	cdevErr1 = cdev_add(&BT_cdev_sco, devID_sco, 1);
	if (cdevErr1)
		goto err1;
	cdevErr2 = cdev_add(&BT_cdevfwlog, devIDfwlog, 1);
	if (cdevErr2)
		goto err1;
	BTUSB_INFO("%s: %s driver(major %d) installed.", __func__, "BT_chrdev", BT_major);
	BTUSB_INFO("%s: %s driver(major %d) installed.", __func__, "BT_chrdevfwlog", BT_majorfwlog);
	BTUSB_INFO("%s: %s driver(major %d) installed.", __func__, "BT_cdev_sco", BT_major_sco);
	pBTClass = class_create(THIS_MODULE, "BT_chrdev");
	if (IS_ERR(pBTClass)) {
		BTUSB_ERR("class create fail, error code(%ld)", PTR_ERR(pBTClass));
		goto err1;
	}

	pBTDev = device_create(pBTClass, NULL, devID, NULL, "stpbt");
	if (IS_ERR(pBTDev)) {
		BTUSB_ERR("device create fail, error code(%ld)", PTR_ERR(pBTDev));
		goto err2;
	}

	pBTDevfwlog = device_create(pBTClass, NULL, devIDfwlog, NULL, "stpbtfwlog");
	if (IS_ERR(pBTDevfwlog)) {
		BTUSB_ERR("device(stpbtfwlog) create fail, error code(%ld)", PTR_ERR(pBTDevfwlog));
		goto err2;
	}

	pBTDev_sco = device_create(pBTClass, NULL, devID_sco, NULL, "stpbt_sco");
	if (IS_ERR(pBTDev_sco)) {
		BTUSB_ERR("device(stpbt_sco) create fail, error code(%ld)", PTR_ERR(pBTDev_sco));
		goto err2;
	}
	BTUSB_INFO("%s: BT_major %d, BT_majorfwlog %d", __func__, BT_major, BT_majorfwlog);
	BTUSB_INFO("%s: devID %d, devIDfwlog %d", __func__, devID, devIDfwlog);
	BTUSB_ERR("%s: BT_major_sco %d, devID_sco %d", __func__, BT_major_sco, devID_sco);
	g_devIDfwlog = devIDfwlog;
	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_INIT);
	FOPS_MUTEX_UNLOCK();

	/* init wait queue */
	init_waitqueue_head(&(inq));
	init_waitqueue_head(&(fw_log_inq));
	init_waitqueue_head(&(inq_isoc));

	/* register system power off callback function. */
	do {
		typedef void (*func_ptr) (int (*f) (void));
		char *func_name = "RegisterPdwncCallback";
		func_ptr pFunc = (func_ptr) kallsyms_lookup_name(func_name);

		if (pFunc) {
			BTUSB_INFO("%s: Register Pdwnc callback success.", __func__);
			pFunc(&btmtk_usb_standby);
		} else
			BTUSB_WARN("%s: No Exported Func Found [%s], just skip!", __func__, func_name);
	} while (0);

	/* allocate buffers. */
	if (btmtk_usb_allocate_memory() < 0) {
		BTUSB_ERR("%s: allocate memory failed!", __func__);
		goto err2;
	}

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
	USB_MUTEX_UNLOCK();
#if (SUPPORT_UNIFY_WOBLE & SUPPORT_ANDROID)
	wake_lock_init(&g_data->woble_wlock, WAKE_LOCK_SUSPEND, "btmtk_woble_wakelock");
#endif
	spin_lock_init(&g_data->fwlog_lock);
	skb_queue_head_init(&g_data->fwlog_queue);
	spin_lock_init(&g_data->isoc_lock);
	skb_queue_head_init(&g_data->isoc_in_queue);

	BTUSB_INFO("%s: end", __func__);
	return 0;

err2:
	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}

	btmtk_usb_free_memory();

err1:
	if (cdevErr0 < 0)
		cdev_del(&BT_cdev);
	if (cdevErr1 < 0)
		cdev_del(&BT_cdev_sco);
	if (cdevErr2 < 0)
		cdev_del(&BT_cdevfwlog);

err0:
	if (ret0 == 0)
		unregister_chrdev_region(devID, 1);
	if (ret1 == 0)
		unregister_chrdev_region(devIDfwlog, 1);
	if (ret2 == 0)
		unregister_chrdev_region(devID_sco, 1);
	BTUSB_ERR("%s: error", __func__);
	return -1;
}

static void btmtk_usb_BT_exit(void)
{
	dev_t dev = MKDEV(BT_major, 0);
	dev_t devIDfwlog = g_devIDfwlog; /* MKDEV(BT_majorfwlog, 0); */
	dev_t devID_sco = MKDEV(BT_major_sco, 0);

	BTUSB_INFO("%s: BT_major %d, BT_majorfwlog %d", __func__, BT_major, BT_majorfwlog);
	BTUSB_INFO("%s: dev %d, devIDfwlog %d", __func__, dev, devIDfwlog);

	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_UNKNOWN);
	FOPS_MUTEX_UNLOCK();
#if (SUPPORT_UNIFY_WOBLE & SUPPORT_ANDROID)
	if (g_data)
		wake_lock_destroy(&g_data->woble_wlock);
	else
		BTUSB_ERR("%s:g_data is NULL, no destroy woble_wlock", __func__);
#endif

	if (pBTDev_sco) {
		device_destroy(pBTClass, devID_sco);
		pBTDev_sco = NULL;
	}

	if (pBTDevfwlog) {
		device_destroy(pBTClass, devIDfwlog);
		pBTDevfwlog = NULL;
	}

	if (pBTDev) {
		device_destroy(pBTClass, dev);
		pBTDev = NULL;
	}

	if (pBTClass) {
		class_destroy(pBTClass);
		pBTClass = NULL;
	}

	cdev_del(&BT_cdev_sco);
	unregister_chrdev_region(devID_sco, 1);

	cdev_del(&BT_cdevfwlog);
	unregister_chrdev_region(devIDfwlog, 1);

	cdev_del(&BT_cdev);
	unregister_chrdev_region(dev, 1);

	btmtk_usb_free_memory();

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_UNKNOWN);
	USB_MUTEX_UNLOCK();

	if (g_proc_dir != 0) {
		remove_proc_entry("bt_fw_version", g_proc_dir);
		remove_proc_entry("stpbt", NULL);
		BTUSB_INFO("%s: BT_proc node removed.", __func__);
	}
	g_proc_dir = 0;

	BTUSB_INFO("%s: BT_chrdev driver removed.", __func__);
}

static int btmtk_usb_handle_resume(void)
{
	int ret = -1;

	BTUSB_INFO("%s", __func__);
	if (is_support_unify_woble(g_data)) {
		ret = btmtk_usb_unify_woble_wake_up();
		if (ret)
			return WOBLE_FAIL;
	}

	if (btmtk_usb_start_traffic())
		return -1;

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
	USB_MUTEX_UNLOCK();

	BTUSB_DBG("%s: call inq", __func__);
	wake_up_interruptible(&inq);
	return 0;
}

static int btmtk_usb_wait_until_event(u8 *wait_event, int wait_event_len, int total_timeout,
		int event_interval_timeout, int hci)
{
	/** return 0 if compare successfully., < 0 if error
	 *  hci: 1 - Send the excepted event to stack
	 */
	int ret = -1, len = 0;
	unsigned long comp_event_timo = 0, start_time = 0;

	start_time = jiffies;
	/* check HCI event */
	comp_event_timo = jiffies + msecs_to_jiffies(total_timeout);
	do {
		memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
		ret = usb_interrupt_msg(g_data->udev, usb_rcvintpipe(g_data->udev, 1),
			g_data->io_buf, USB_IO_BUF_SIZE, &len, event_interval_timeout);
		if (ret == (-ETIMEDOUT)) {
			continue;
		} else if (ret < 0) {
			BTUSB_ERR("%s: ret(%d) error break", __func__, ret);
			break;
		}

		if (len >= 2 && g_data->io_buf[1] + 2 != len) {
			BTUSB_WARN("%s: Ignore incorrect format packet:%02X %02X",
					__func__, g_data->io_buf[0], g_data->io_buf[1]);
			continue;
		}

		/* check if receive expected event */
		if (len >= wait_event_len) {
			if (memcmp(wait_event, g_data->io_buf, wait_event_len) == 0) {
				if (hci) /* 1: save it as vendor event */
					btmtk_usb_add_to_hci_log(g_data->io_buf, len, HCI_EVENT_PKT);
				return 0;
			}
		}
		/* maybe this is a standard or picus event */
		btmtk_usb_dispatch_event(g_data->io_buf, len);
	} while (time_before(jiffies, comp_event_timo));

	BTUSB_ERR("%s: Get compelete event fail %d", __func__, ret);
	return ret;
}

static int btmtk_usb_set_Woble_APCF_filter_parameter(void)
{
	int ret = -1;
	u8 cmd[] = { 0x57, 0xfd, 0x0a, 0x01, 0x00, 0x5a, 0x20, 0x00, 0x20, 0x00, 0x01, 0x80, 0x00 };
	u8 event_complete[] = { 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00, 0x01/*, 00, 63*/ };

	BTUSB_INFO("%s: begin", __func__);
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event_complete, sizeof(event_complete));
	if (ret < 0)
		BTUSB_ERR("%s: end ret %d", __func__, ret);
	else
		ret = 0;

	BTUSB_INFO("%s: end ret=%d", __func__, ret);
	return ret;
}

static int btmtk_usb_send_read_BDADDR_cmd(void)
{
	u8 cmd[] = { 0x09, 0x10, 0x00 };
	u8 event[] = { 0x0E, 0x0A, 0x01, 0x09, 0x10, 0x00, /* AA, BB, CC, DD, EE, FF */ };
	int i;
	int ret = -1;

	BTUSB_INFO("%s: begin", __func__);
	if (g_data == NULL || g_data->io_buf == NULL) {
		BTUSB_ERR("%s: Incorrect g_data", __func__);
		return -1;
	}

	for (i = 0; i < BD_ADDRESS_SIZE; i++) {
		if (g_data->bdaddr[i] != 0) {
			ret = 0;
			goto done;
		}
	}

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
		return ret;
	}
	ret = 0;

	for (i = 0; i < BD_ADDRESS_SIZE; i++)
		g_data->bdaddr[i] = g_data->io_buf[6 + i];

done:
	BTUSB_INFO("%s: Local BDADDR: %02X:%02X:%02X:%02X:%02X:%02X", __func__,
		g_data->bdaddr[5], g_data->bdaddr[4], g_data->bdaddr[3],
		g_data->bdaddr[2], g_data->bdaddr[1], g_data->bdaddr[0]);
	return ret;
}

/**
 * Set APCF manufacturer data and filter parameter
 */
static int btmtk_usb_set_Woble_APCF(void)
{
	int ret = -1;
	int i = 0;
	u8 manufactur_data[] = { 0x57, 0xfd, 0x27, 0x06, 0x00, 0x5a,
		0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x43, 0x52, 0x4B, 0x54, 0x4D,
		0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	u8 event[] = { 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00, /* 0x06 00 63 */ };

	BTUSB_INFO("%s: woble_setting_apcf[0].length %d",
			__func__, g_data->woble_setting_apcf[0].length);

	/* start to send apcf cmd from woble setting file */
	if (g_data->woble_setting_apcf[0].length) {
		for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
			if (!g_data->woble_setting_apcf[i].length)
				continue;

			BTUSB_INFO("%s: apcf_fill_mac[%d].content[0] = 0x%02x", __func__, i,
					g_data->woble_setting_apcf_fill_mac[i].content[0]);
			BTUSB_INFO("%s: apcf_fill_mac_location[%d].length = %d", __func__, i,
					g_data->woble_setting_apcf_fill_mac_location[i].length);

			if ((g_data->woble_setting_apcf_fill_mac[i].content[0] == 1) &&
				g_data->woble_setting_apcf_fill_mac_location[i].length) {
				/* need add BD addr to apcf cmd */
				memcpy(g_data->woble_setting_apcf[i].content +
					(*g_data->woble_setting_apcf_fill_mac_location[i].content),
					g_data->bdaddr, BD_ADDRESS_SIZE);
				BTUSB_INFO("%s: apcf[%d], add local BDADDR to location %d", __func__, i,
						(*g_data->woble_setting_apcf_fill_mac_location[i].content));
			}

			BTUSB_INFO_RAW(g_data->woble_setting_apcf[i].content, g_data->woble_setting_apcf[i].length,
				"Send woble_setting_apcf[%d] ", i);

			ret = btmtk_usb_send_hci_cmd(g_data->woble_setting_apcf[i].content,
					g_data->woble_setting_apcf[i].length, event, sizeof(event));
			if (ret < 0) {
				BTUSB_ERR("%s: manufactur_data error ret %d", __func__, ret);
				return ret;
			}
		}
	} else { /* use default */
		BTUSB_INFO("%s: use default manufactur data", __func__);
		memcpy(manufactur_data + 9, g_data->bdaddr, BD_ADDRESS_SIZE);
		ret = btmtk_usb_send_hci_cmd(
			manufactur_data, sizeof(manufactur_data), event, sizeof(event));
		if (ret < 0) {
			BTUSB_ERR("%s: manufactur_data error ret %d", __func__, ret);
			return ret;
		}

		ret = btmtk_usb_set_Woble_APCF_filter_parameter();
	}
	BTUSB_INFO("%s: end ret=%d", __func__, ret);
	return 0;
}

static int btmtk_usb_handle_leaving_WoBLE_state(void)
{
	int ret = -1;

	if (!is_support_unify_woble(g_data))
		return 0;

	BTUSB_INFO("%s: woble_setting_radio_on.length %d", __func__,
			g_data->woble_setting_radio_on.length);
	if (g_data->woble_setting_radio_on.length) { /* start to send radio off cmd from woble setting file */
		if (g_data->woble_setting_radio_on.length) {
			BTUSB_INFO_RAW(g_data->woble_setting_radio_on.content,
					g_data->woble_setting_radio_on.length, "send radio on");

			ret = btmtk_usb_send_hci_cmd(g_data->woble_setting_radio_on.content,
					g_data->woble_setting_radio_on.length,
					g_data->woble_setting_radio_on_status_event.content,
					g_data->woble_setting_radio_on_status_event.length);
			if (ret < 0) {
				BTUSB_ERR("%s: btmtk_usb_send_hci_cmd return fail %d", __func__, ret);
				return ret;
			}

			if (g_data->woble_setting_radio_on_comp_event.length) {
				BTUSB_INFO("%s: check woble_setting_radio_on_comp_event", __func__);
				ret = btmtk_usb_wait_until_event(g_data->woble_setting_radio_on_comp_event.content,
						g_data->woble_setting_radio_on_comp_event.length,
						USB_INTR_MSG_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 1);
				if (ret < 0) {
					BTUSB_ERR("%s: woble_setting_radio_on_comp_event ret:%d", __func__, ret);
					return ret;
				}
			}
		}
	} else { /* use default */
		BTUSB_WARN("%s: use default radio on cmd", __func__);
		ret = btmtk_usb_send_leave_woble_suspend_cmd();
		if (ret)
			return ret;
	}
	return ret;
}

static int btmtk_usb_del_Woble_APCF_index(void)
{
	int ret = -1;
	u8 cmd[] = { 0x57, 0xfd, 0x03, 0x01, 0x01, 0x5a };
	u8 event[] = { 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00, 0x01, /* 00, 63 */ };

	BTUSB_INFO("%s", __func__);
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: Got error %d", __func__, ret);
		return ret;
	}
	return ret;
}

/* Check power status, if power is off, try to set power on */
static int btmtk_usb_reset_power_on(void)
{
	if (is_mt7668(g_data) || is_mt7663(g_data)) {
		int retry = 0;

		while ((g_data->is_mt7668_dongle_state == BTMTK_USB_7668_DONGLE_STATE_POWERING_ON ||
			g_data->is_mt7668_dongle_state == BTMTK_USB_7668_DONGLE_STATE_POWERING_OFF)
				&& retry++ < 10) {
			BTUSB_INFO("%s: dongle state is POWERING ON or OFF.", __func__);
			msleep(100);
		}
		if (g_data->is_mt7668_dongle_state == BTMTK_USB_7668_DONGLE_STATE_POWER_OFF) {
			g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;
			if (btmtk_usb_send_wmt_power_on_cmd_7668() < 0)
				return -1;
			if (btmtk_usb_send_hci_tci_set_sleep_cmd_7668() < 0)
				return -1;
			if (btmtk_usb_send_hci_reset_cmd() < 0)
				return -1;
			g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_ON;
		}

		if (g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_POWER_ON) {
			BTUSB_WARN("%s: end of Incorrect state:%d", __func__, g_data->is_mt7668_dongle_state);
			return -EBADFD;
		}
		BTUSB_INFO("%s: 7668 end success", __func__);
	}
	return 0;
}

static int btmtk_usb_send_unify_woble_suspend_cmd(void)
{
	int ret = 0;	/* if successful, 0 */
	u8 cmd[] = { 0xC9, 0xFC, 0x14, 0x01, 0x20, 0x02, 0x00, 0x01,
		0x02, 0x01, 0x00, 0x05, 0x10, 0x01, 0x00, 0x40, 0x06,
		0x02, 0x40, 0x5A, 0x02, 0x41, 0x0F };
	u8 status[] = { 0x0F, 0x04, 0x00, 0x01, 0xC9, 0xFC };
	u8 comp_event[] = { 0xE6, 0x02, 0x08, 0x00 };

	BTUSB_DBG("%s: begin", __func__);
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), status, sizeof(status));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
		return ret;
	}

	ret = btmtk_usb_wait_until_event(comp_event, sizeof(comp_event),
			WOBLE_COMP_EVENT_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 1);
	if (ret < 0)
		BTUSB_ERR("%s: comp_event return error(%d)", __func__, ret);

	return ret;
}

static int btmtk_usb_send_leave_woble_suspend_cmd(void)
{
	int ret = 0;	/* if successful, 0 */
	u8 cmd[] = { 0xC9, 0xFC, 0x05, 0x01, 0x21, 0x02, 0x00, 0x00 };
	u8 status[] = { 0x0F, 0x04, 0x00, 0x01, 0xC9, 0xFC };
	u8 comp_event[] = { 0xE6, 0x02, 0x08, 0x01 };

	BTUSB_INFO("%s: begin", __func__);
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), status, sizeof(status));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
		return ret;
	}

	ret = btmtk_usb_wait_until_event(comp_event, sizeof(comp_event),
			WOBLE_COMP_EVENT_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 1);
	if (ret < 0)
		BTUSB_ERR("%s: comp_event return error(%d)", __func__, ret);

	return ret;
}

static int btmtk_usb_handle_entering_WoBLE_state(void)
{
	int ret = -1;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	BTUSB_INFO("%s: begin", __func__);

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	/*if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not open yet(%d)!, return", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}*/
	FOPS_MUTEX_UNLOCK();

	/* Power on first if state is power off */
	ret = btmtk_usb_reset_power_on();
	if (ret) {
		BTUSB_INFO("%s: reset power_on fail return", __func__);
		goto Finish;
	}

	if (is_support_unify_woble(g_data)) {
		ret = btmtk_usb_send_get_vendor_cap();
		if (ret)
			goto Finish;
		ret = btmtk_usb_send_read_BDADDR_cmd();
		if (ret)
			goto Finish;
		ret = btmtk_usb_set_Woble_APCF();
		if (ret)
			goto Finish;

		BTUSB_INFO("%s: woble_setting_radio_off.length %d", __func__,
				g_data->woble_setting_radio_off.length);
		if (g_data->woble_setting_radio_off.length && is_support_unify_woble(g_data)) {
			/* start to send radio off cmd from woble setting file */
			BTUSB_INFO_RAW(g_data->woble_setting_radio_off.content,
					g_data->woble_setting_radio_off.length, "Send radio off");

			ret = btmtk_usb_send_hci_cmd(g_data->woble_setting_radio_off.content,
					g_data->woble_setting_radio_off.length,
					g_data->woble_setting_radio_off_status_event.content,
					g_data->woble_setting_radio_off_status_event.length);
			if (ret < 0) {
				BTUSB_ERR("%s: btmtk_usb_send_hci_cmd return fail %d", __func__, ret);
				goto Finish;
			}

			if (g_data->woble_setting_radio_off_comp_event.length) {
				BTUSB_INFO("%s: check radio_off_comp_event", __func__);
				ret = btmtk_usb_wait_until_event(
						g_data->woble_setting_radio_off_comp_event.content,
						g_data->woble_setting_radio_off_comp_event.length,
						USB_INTR_MSG_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 1);
				if (ret < 0) {
					BTUSB_ERR("%s: radio_off_complete_event ret:%d", __func__, ret);
					goto Finish;
				}
			}
		} else { /* use default */
			BTUSB_INFO("%s: use default radio off cmd", __func__);
			ret = btmtk_usb_send_unify_woble_suspend_cmd();
			if (ret)
				goto Finish;
		}
	} else {
		ret = btmtk_usb_send_woble_suspend_cmd();
		return ret;
	}

Finish:
	if (is_support_unify_woble(g_data)) {
		if (ret) {
			g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;
			btmtk_usb_woble_wake_lock(g_data);
		} else
			g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_WOBLE;
	}
	BTUSB_INFO("%s: end", __func__);
	return ret;
}

/*============================================================================*/
/* Internal Functions : Chip Related */
/*============================================================================*/
#define ______________________________________Internal_Functions_Chip_Related
/**
 * Only for load rom patch function, tmp_str[15] is '\n'
 */
#define SHOW_FW_DETAILS(s)							\
	BTUSB_INFO("%s: %s = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", __func__, s,	\
			tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],		\
			tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],		\
			tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],	\
			tmp_str[12], tmp_str[13], tmp_str[14]/*, tmp_str[15]*/)

#if SUPPORT_MT7662
static int btmtk_usb_load_rom_patch_7662(void)
{
	u32 loop = 0;
	u32 value;
	s32 sent_len;
	int ret = 0;
	u16 total_checksum = 0;
	struct urb *urb;
	u32 patch_len = 0;
	u32 cur_len = 0;
	dma_addr_t data_dma;
	struct completion sent_to_mcu_done;
	int first_block = 1;
	u8 phase;
	void *buf;
	u8 *pos;
	char *tmp_str;
	unsigned int pipe = usb_sndbulkpipe(g_data->udev,
						g_data->
						bulk_tx_ep->bEndpointAddress);

load_patch_protect:
	btmtk_usb_switch_iobase_7662(WLAN);
	btmtk_usb_io_read32_7662(SEMAPHORE_03, &value);
	loop++;

	if ((value & 0x01) == 0x00) {
		if (loop < 1000) {
			mdelay(1);
			goto load_patch_protect;
		} else {
			BTUSB_WARN("%s: WARNING! Can't get semaphore! Continue", __func__);
		}
	}

	btmtk_usb_switch_iobase_7662(SYSCTL);

	btmtk_usb_io_write32_7662(0x1c, 0x30);

	btmtk_usb_switch_iobase_7662(WLAN);

	urb = usb_alloc_urb(0, GFP_KERNEL);

	if (!urb) {
		ret = -ENOMEM;
		goto error0;
	}

	buf = usb_alloc_coherent(g_data->udev, UPLOAD_PATCH_UNIT, GFP_KERNEL, &data_dma);
	if (!buf) {
		ret = -ENOMEM;
		goto error1;
	}

	pos = buf;
	btmtk_usb_load_code_from_bin(&g_data->rom_patch,
					 g_data->rom_patch_bin_file_name,
					 &g_data->udev->dev,
					 &g_data->rom_patch_len);

	if (!g_data->rom_patch) {
		BTUSB_ERR("%s: please assign a rom patch(/etc/firmware/%s)or(/lib/firmware/%s)",
				__func__, g_data->rom_patch_bin_file_name,
				 g_data->rom_patch_bin_file_name);

		ret = -1;
		goto error2;
	}

	tmp_str = g_data->rom_patch;
	memcpy(fw_version_str, g_data->rom_patch, FW_VERSION_BUF_SIZE);
	SHOW_FW_DETAILS("FW Version");
	SHOW_FW_DETAILS("build Time");

	/* check ROM patch if upgrade */
	btmtk_usb_io_read32_7662(CLOCK_CTL, &value);
	if ((value & 0x01) == 0x01) {
		BTUSB_INFO("%s: no need to load rom patch", __func__);
		if (!is_mt7662T(g_data))
			btmtk_usb_send_dummy_bulk_out_packet_7662();
		goto error2;
	}

	tmp_str = g_data->rom_patch + 16;
	BTUSB_INFO("%s: platform = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	tmp_str = g_data->rom_patch + 20;
	BTUSB_INFO("%s: HW/SW version = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	tmp_str = g_data->rom_patch + 24;
	BTUSB_INFO("%s: Patch version = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	do {
		typedef void (*pdwnc_func) (u8 fgReset);
		char *pdwnc_func_name = "PDWNC_SetBTInResetState";
		pdwnc_func pdwndFunc = NULL;

		pdwndFunc = (pdwnc_func) kallsyms_lookup_name(pdwnc_func_name);

		if (pdwndFunc) {
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, pdwnc_func_name, 1);
			pdwndFunc(1);
		} else {
			BTUSB_WARN("%s: No Exported Func Found [%s]", __func__, pdwnc_func_name);
		}
	} while (0);

	BTUSB_INFO("%s: rom patch %s loading...", __func__, g_data->rom_patch_bin_file_name);

	init_completion(&sent_to_mcu_done);

	cur_len = 0x00;
	patch_len = g_data->rom_patch_len - PATCH_INFO_SIZE;
	BTUSB_INFO("%s: patch_len = %d", __func__, patch_len);

	/* loading rom patch */
	while (1) {
		s32 sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;

		sent_len = (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

		if (sent_len > 0) {
			if (first_block == 1) {
				if (sent_len < sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE1;
				first_block = 0;
			} else if (sent_len == sent_len_max) {
				if (patch_len - cur_len == sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE2;
			} else {
				phase = PATCH_PHASE3;
			}

			/* prepare HCI header */
			pos[0] = 0x6F;
			pos[1] = 0xFC;
			pos[2] = (sent_len + 5) & 0xFF;
			pos[3] = ((sent_len + 5) >> 8) & 0xFF;

			/* prepare WMT header */
			pos[4] = 0x01;
			pos[5] = 0x01;
			pos[6] = (sent_len + 1) & 0xFF;
			pos[7] = ((sent_len + 1) >> 8) & 0xFF;

			pos[8] = phase;

			memcpy(&pos[9], g_data->rom_patch + PATCH_INFO_SIZE + cur_len, sent_len);

			BTUSB_INFO("%s: sent_len = %d, cur_len = %d, phase = %d", __func__, sent_len, cur_len, phase);

			usb_fill_bulk_urb(urb,
					g_data->udev,
					pipe,
					buf,
					sent_len + PATCH_HEADER_SIZE,
					(usb_complete_t)btmtk_usb_load_rom_patch_complete,
					&sent_to_mcu_done);

			urb->transfer_dma = data_dma;
			urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

			ret = usb_submit_urb(urb, GFP_KERNEL);

			if (ret)
				goto error2;

			if (!wait_for_completion_timeout
				(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
				usb_kill_urb(urb);
				BTUSB_ERR("%s: upload rom_patch timeout", __func__);
				ret = -ETIME;
				goto error2;
			}

			cur_len += sent_len;
			mdelay(1);

		} else {
			BTUSB_INFO("%s: loading rom patch... Done", __func__);
			break;
		}
	}

	mdelay(20);
	ret = btmtk_usb_get_rom_patch_result();
	mdelay(1);

	/* Send Checksum request */
	total_checksum = btmtk_usb_checksum16_7662(g_data->rom_patch + PATCH_INFO_SIZE, patch_len);
	btmtk_usb_chk_crc_7662(patch_len);

	mdelay(1);

	if (total_checksum != btmtk_usb_get_crc_7662()) {
		BTUSB_ERR("checksum fail!, local(0x%x) <> fw(0x%x)", total_checksum, btmtk_usb_get_crc_7662());
		ret = -1;
		goto error2;
	}

	/* send check rom patch result request */
	mdelay(1);
	btmtk_usb_send_hci_check_rom_patch_result_cmd_7662();

	/* CHIP_RESET */
	mdelay(1);
	ret = btmtk_usb_send_wmt_reset_cmd();

	/* BT_RESET */
	mdelay(1);
	btmtk_usb_send_hci_reset_cmd();

	/* Enable BT Low Power */
	mdelay(1);
	btmtk_usb_send_hci_low_power_cmd_7662(TRUE);

	/* for WoBLE/WoW low power */
	mdelay(1);
	btmtk_usb_send_hci_set_ce_cmd_7662();

	mdelay(1);
	btmtk_usb_send_hci_set_tx_power_cmd_7662();

error2:
	usb_free_coherent(g_data->udev, UPLOAD_PATCH_UNIT, buf, data_dma);
error1:
	usb_free_urb(urb);
error0:
	btmtk_usb_io_write32_7662(SEMAPHORE_03, 0x1);

	do {
		typedef void (*pdwnc_func) (u8 fgReset);
		char *pdwnc_func_name = "PDWNC_SetBTInResetState";
		pdwnc_func pdwndFunc = NULL;

		pdwndFunc = (pdwnc_func) kallsyms_lookup_name(pdwnc_func_name);

		if (pdwndFunc) {
			BTUSB_INFO("%s: Invoke %s(%d)", __func__, pdwnc_func_name, 0);
			pdwndFunc(0);
		} else
			BTUSB_WARN("%s: No Exported Func Found [%s]", __func__, pdwnc_func_name);
	} while (0);

	return ret;
}

static int btmtk_usb_io_read32_7662(u32 reg, u32 *val)
{
	int ret = -1;
	u8 request = g_data->r_request;

	memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
	ret = usb_control_msg(g_data->udev, usb_rcvctrlpipe(g_data->udev, 0),
			request, DEVICE_VENDOR_REQUEST_IN, 0x0, reg,
			g_data->io_buf, sizeof(u32), USB_CTRL_IO_TIMO);

	if (ret < 0) {
		*val = 0xffffffff;
		BTUSB_ERR("%s: error(%d), reg=%x, value=%x", __func__, ret, reg, *val);
		return ret;
	}

	memmove(val, g_data->io_buf, sizeof(u32));
	*val = le32_to_cpu(*val);

	return 0;
}

static int btmtk_usb_io_write32_7662(u32 reg, u32 val)
{
	int ret;
	u16 value, index;
	u8 request = g_data->w_request;

	index = (u16) reg;
	value = val & 0x0000ffff;

	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0),
				request, DEVICE_VENDOR_REQUEST_OUT, value, index,
				NULL, 0, USB_CTRL_IO_TIMO);

	if (ret < 0) {
		BTUSB_ERR("%s: error(%d), reg=%x, value=%x", __func__, ret, reg, val);
		return ret;
	}

	index = (u16) (reg + 2);
	value = (val & 0xffff0000) >> 16;

	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0),
				request, DEVICE_VENDOR_REQUEST_OUT, value, index,
				NULL, 0, USB_CTRL_IO_TIMO);

	if (ret < 0) {
		BTUSB_ERR("%s: error(%d), reg=%x, value=%x", __func__, ret, reg, val);
		return ret;
	}

	if (ret > 0)
		ret = 0;

	return ret;
}

static int btmtk_usb_switch_iobase_7662(int base)
{
	int ret = 0;

	switch (base) {
	case SYSCTL:
		g_data->w_request = 0x42;
		g_data->r_request = 0x47;
		break;
	case WLAN:
		g_data->w_request = 0x02;
		g_data->r_request = 0x07;
		break;

	default:
		return -EINVAL;
	}

	return ret;
}

static u16 btmtk_usb_checksum16_7662(u8 *pData, int len)
{
	u32 sum = 0;

	while (len > 1) {
		sum += *((u16 *) pData);

		pData = pData + 2;

		if (sum & 0x80000000)
			sum = (sum & 0xFFFF) + (sum >> 16);

		len -= 2;
	}

	if (len)
		sum += *((u8 *) pData);

	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return (u16)(~sum);
}

static int btmtk_usb_chk_crc_7662(u32 checksum_len)
{
	int ret = 0;

	BT_DBG("%s", __func__);

	memmove(g_data->io_buf, &g_data->rom_patch_offset, 4);
	memmove(&g_data->io_buf[4], &checksum_len, 4);

	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0), 0x1,
				DEVICE_VENDOR_REQUEST_OUT, 0x20, 0x00,
				g_data->io_buf, 8, USB_CTRL_IO_TIMO);

	if (ret < 0)
		BTUSB_ERR("%s: error(%d)", __func__, ret);

	return ret;
}

static u16 btmtk_usb_get_crc_7662(void)
{
	int ret = 0;
	u16 crc, count = 0;

	while (1) {
		ret = usb_control_msg(g_data->udev,
					usb_rcvctrlpipe(g_data->udev, 0), 0x01,
					DEVICE_VENDOR_REQUEST_IN, 0x21, 0x00,
					g_data->io_buf, USB_IO_BUF_SIZE,
					USB_CTRL_IO_TIMO);

		if (ret < 0) {
			crc = 0xFFFF;
			BTUSB_ERR("%s: error(%d)", __func__, ret);
		}

		memmove(&crc, g_data->io_buf, 2);

		crc = le16_to_cpu(crc);

		if (crc != 0xFFFF)
			break;

		mdelay(1);

		if (count++ > 100) {
			BTUSB_WARN("Query CRC over %d times", count);
			break;
		}
	}

	return crc;
}

static int btmtk_usb_send_hci_set_tx_power_cmd_7662(void)
{
	u8 cmd[] = { 0x79, 0xFC, 0x06, 0x07, 0x80, 0x00, 0x06, 0x07, 0x07 };
	u8 event[] = { 0x0E, 0x04, 0x01, 0x79, 0xFC, 0x00 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_hci_check_rom_patch_result_cmd_7662(void)
{
	u8 cmd[] = { 0xD1, 0xFC, 0x04, 0x00, 0xE2, 0x40, 0x00 };
	u8 event[] = { 0x0E, 0x08, 0x01, 0xD1, 0xFC, 0x00 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);

	} else if (ret == sizeof(event) + 4) {
		if (g_data->io_buf[6] == 0 &&
			g_data->io_buf[7] == 0 &&
			g_data->io_buf[8] == 0 &&
			g_data->io_buf[9] == 0) {
			BTUSB_WARN("Check rom patch result: NG");
			return -1;

		} else {
			BTUSB_INFO("Check rom patch result: OK");
			ret = 0;
		}

	} else {
		BTUSB_ERR("%s: failed, incorrect response length(%d)", __func__, ret);
		return -1;
	}

	return ret;
}

static int btmtk_usb_send_hci_radio_on_cmd_7662(void)
{
	u8 cmd[] = { 0xC9, 0xFC, 0x02, 0x01, 0x01 };
	u8 event[] = { 0xE6, 0x02, 0x08, 0x01 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_hci_set_ce_cmd_7662(void)
{
	u8 cmd[] = { 0xD1, 0xFC, 0x04, 0x0C, 0x07, 0x41, 0x00 };
	u8 event[] = { 0x0E, 0x08, 0x01, 0xD1, 0xFC, 0x00 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);

	} else if (ret == sizeof(event) + 4) {
		if (g_data->io_buf[6] & 0x01) {
			BTUSB_WARN("warning, 0x41070c[0] is 1!");
			ret = 0;
		} else {
			/* write 0x41070C[0] to 1 */
			u8 cmd2[11] = { 0xD0, 0xFC, 0x08, 0x0C, 0x07, 0x41, 0x00 };

			cmd2[7] = g_data->io_buf[6] | 0x01;
			cmd2[8] = g_data->io_buf[7];
			cmd2[9] = g_data->io_buf[8];
			cmd2[10] = g_data->io_buf[9];

			ret = btmtk_usb_send_hci_cmd(cmd2, sizeof(cmd2), NULL, 0);
			if (ret < 0) {
				BTUSB_ERR("%s: write 0x41070C failed(%d)", __func__, ret);
			} else {
				BTUSB_INFO("%s: OK", __func__);
				ret = 0;
			}
		}
	} else {
		BTUSB_ERR("%s: failed, incorrect response length(%d)", __func__, ret);
		return -1;
	}

	return ret;
}

static int btmtk_usb_send_hci_radio_off_cmd_7662(void)
{
	u8 cmd[] = { 0xC9, 0xFC, 0x02, 0x01, 0x00 };
	u8 event[] = { 0xE6, 0x02, 0x08, 0x00 };	/* unexpected opcode */
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_hci_low_power_cmd_7662(bool enable)
{
	/* default for disable */
	u8 cmd[] = { 0x40, 0xFC, 0x00 };
	u8 event[] = { 0x0E, 0x04, 0x01, 0x40, 0xFC, 0x00 };
	int ret = -1;	/* if successful, 0 */

	/* for enable */
	if (enable == TRUE) {
		cmd[0] = 0x41;
		event[3] = 0x41;
	}
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static void btmtk_usb_send_dummy_bulk_out_packet_7662(void)
{
	int ret = 0;
	int actual_len;
	unsigned int pipe;

	pipe = usb_sndbulkpipe(g_data->udev, g_data->bulk_tx_ep->bEndpointAddress);

	memset(g_data->o_usb_buf, 0, 8);
	ret = usb_bulk_msg(g_data->udev, pipe, g_data->o_usb_buf, 8, &actual_len, 100);
	if (ret)
		BTUSB_ERR("%s: submit dummy bulk out failed!", __func__);
	else
		BTUSB_INFO("%s: 1. OK", __func__);

	memset(g_data->o_usb_buf, 0, 8);
	ret = usb_bulk_msg(g_data->udev, pipe, g_data->o_usb_buf, 8, &actual_len, 100);
	if (ret)
		BTUSB_ERR("%s: submit dummy bulk out failed!", __func__);
	else
		BTUSB_INFO("%s: 2. OK", __func__);
}
#endif /* SUPPORT_MT7662 */

#if SUPPORT_MT7668 || SUPPORT_MT7663
static int btmtk_usb_io_read32_7668(u32 reg, u32 *val)
{
	int ret = -1;
	__le16 reg_high;
	__le16 reg_low;

	reg_high = ((reg >> 16) & 0xffff);
	reg_low = (reg & 0xffff);

	memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
	ret = usb_control_msg(g_data->udev, usb_rcvctrlpipe(g_data->udev, 0),
			0x63,				/* bRequest */
			DEVICE_VENDOR_REQUEST_IN,	/* bRequestType */
			reg_high,			/* wValue */
			reg_low,			/* wIndex */
			g_data->io_buf,
			sizeof(u32), USB_CTRL_IO_TIMO);

	if (ret < 0) {
		*val = 0xffffffff;
		BTUSB_ERR("%s: error(%d), reg=%x, value=%x", __func__, ret, reg, *val);
		return ret;
	}

	memmove(val, g_data->io_buf, sizeof(u32));
	*val = le32_to_cpu(*val);

	return 0;
}

static bool btmtk_usb_check_need_load_rom_patch_7668(void)
{
	/* TRUE: need load patch., FALSE: do not need */
	u8 cmd[] = { 0x6F, 0xFC, 0x05, 0x01, 0x17, 0x01, 0x00, 0x01 };
	u8 event[] = { 0xE4, 0x05, 0x02, 0x17, 0x01, 0x00, /* 0x02 */ };	/* event[6] is key */
	int ret = -1;

	BTUSB_DBG_RAW(cmd, sizeof(cmd), "%s: Send CMD:", __func__);
	ret = btmtk_usb_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 20, 0);
	/* can't get correct event, need load patch */
	if (ret < 0)
		return TRUE;

	if (ret == sizeof(event) + 1 && (g_data->io_buf[6] == 0x00 || g_data->io_buf[6] == 0x01))
		return FALSE;	/* Do not need */

	return TRUE;
}

static int btmtk_usb_check_bt_power_status_7668(void)
{
	/* TRUE: ON, FALSE: OFF */
	u8 cmd[] = { 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x04, 0x00 };
	u8 event[] = { 0xE4, 0x07, 0x02, 0x06, 0x03, 0x00, 0x00, 0x04 };	/* event[8] is key */
	int ret = -1;
	int retry = 0;

	do {
		ret = btmtk_usb_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 100, 20);
		/* get bt power status failed */
		if (ret < 0)
			return ret;

		if (ret == sizeof(event) + 1) {
			BTUSB_INFO("%s: %02X", __func__, g_data->io_buf[8]);
			/* Check the bit5 firstly for calibration before power status., bit5 since Bora CL58715 */
			if (g_data->io_buf[8] & 0x20) {		/* bit5 1:calibrating, 0:ok*/
				msleep(200);
				BTUSB_WARN("%s: Calibrating check again", __func__);
				continue;
			} else if (g_data->io_buf[8] & 0x04) {	/* bit3 1:BT on, 0:BT off */
				return 1;
			} else {
				return 0;
			}
		} else {
			BTUSB_WARN("%s: got unknown result (%d)", __func__, ret);
			BTUSB_DBG_RAW(g_data->io_buf, ret, "%s:", __func__);
			return -1;
		}
	} while (retry++ < 5);
	BTUSB_ERR("%s: Calibrating check fail", __func__);
	return -1;
}

static int btmtk_usb_load_rom_patch_7668(void)
{
	s32 sent_len;
	int ret = 0;
	struct urb *urb;
	u32 patch_len = 0;
	u32 cur_len = 0;
	dma_addr_t data_dma;
	struct completion sent_to_mcu_done;
	int first_block = 1;
	u8 phase;
	void *buf;
	u8 *pos;
	char *tmp_str;
	unsigned int pipe = usb_sndbulkpipe(g_data->udev, g_data->bulk_tx_ep->bEndpointAddress);

	BTUSB_INFO("%s: begin", __func__);

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		ret = -ENOMEM;
		goto error0;
	}

	buf = usb_alloc_coherent(g_data->udev, UPLOAD_PATCH_UNIT, GFP_KERNEL, &data_dma);
	if (!buf) {
		ret = -ENOMEM;
		goto error1;
	}

	pos = buf;

	btmtk_usb_load_code_from_bin(&g_data->rom_patch,
			g_data->rom_patch_bin_file_name, &g_data->udev->dev,
			&g_data->rom_patch_len);
	if (!g_data->rom_patch) {
		BTUSB_ERR("%s: please assign a rom patch(/etc/firmware/%s)or(/lib/firmware/%s)",
				 __func__, g_data->rom_patch_bin_file_name,
				 g_data->rom_patch_bin_file_name);
		ret = -1;
		goto error2;
	}

	tmp_str = g_data->rom_patch;
	memcpy(fw_version_str, g_data->rom_patch, FW_VERSION_BUF_SIZE);
	SHOW_FW_DETAILS("FW Version");
	SHOW_FW_DETAILS("build Time");

	if (btmtk_usb_check_need_load_rom_patch_7668() == FALSE) {
		BTUSB_INFO("%s: no need to load rom patch", __func__);
		goto error2;
	}

	tmp_str = g_data->rom_patch + 16;
	BTUSB_INFO("%s: platform = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	tmp_str = g_data->rom_patch + 20;
	BTUSB_INFO("%s: HW/SW version = %c%c%c%c", __func__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

	tmp_str = g_data->rom_patch + 24;

	BTUSB_INFO("loading rom patch...");

	init_completion(&sent_to_mcu_done);

	cur_len = 0x00;
	patch_len = g_data->rom_patch_len - PATCH_INFO_SIZE;
	BTUSB_INFO("%s: patch_len = %d", __func__, patch_len);

	BTUSB_INFO("%s: loading rom patch...", __func__);
	/* loading rom patch */
	while (1) {
		s32 sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;
		int status = -1;

		sent_len = (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

		if (sent_len > 0) {
			if (first_block == 1) {
				if (sent_len < sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE1;
				first_block = 0;
			} else if (sent_len == sent_len_max) {
				if (patch_len - cur_len == sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE2;
			} else {
				phase = PATCH_PHASE3;
			}

			/* prepare HCI header */
			pos[0] = 0x6F;
			pos[1] = 0xFC;
			pos[2] = (sent_len + 5) & 0xFF;
			pos[3] = ((sent_len + 5) >> 8) & 0xFF;

			/* prepare WMT header */
			pos[4] = 0x01;
			pos[5] = 0x01;
			pos[6] = (sent_len + 1) & 0xFF;
			pos[7] = ((sent_len + 1) >> 8) & 0xFF;

			pos[8] = phase;

			memcpy(&pos[9], g_data->rom_patch + PATCH_INFO_SIZE + cur_len,
					sent_len);

			BTUSB_INFO("%s: sent_len = %d, cur_len = %d, phase = %d", __func__, sent_len, cur_len, phase);

			usb_fill_bulk_urb(urb,
					g_data->udev,
					pipe,
					buf,
					sent_len + PATCH_HEADER_SIZE,
					(usb_complete_t)btmtk_usb_load_rom_patch_complete,
					&sent_to_mcu_done);

			urb->transfer_dma = data_dma;
			urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

			status = usb_submit_urb(urb, GFP_KERNEL);
			if (status) {
				BTUSB_ERR("%s: submit urb failed (%d)", __func__, status);
				ret = status;
				goto error2;
			}

			if (!wait_for_completion_timeout
				(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
				usb_kill_urb(urb);
				BTUSB_ERR("%s: upload rom_patch timeout", __func__);
				ret = -ETIME;
				goto error2;
			}

			cur_len += sent_len;

			mdelay(1);
			if (btmtk_usb_get_rom_patch_result() < 0)
				goto error2;
			mdelay(1);

		} else {
			BTUSB_INFO("%s: loading rom patch... Done", __func__);
			break;
		}
	}

	/* CHIP_RESET, ROM patch would be reactivated */
	ret = btmtk_usb_send_wmt_reset_cmd();
	g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_OFF;
error2:
	usb_free_coherent(g_data->udev, UPLOAD_PATCH_UNIT, buf, data_dma);
error1:
	usb_free_urb(urb);
error0:
	BTUSB_INFO("btmtk_usb_load_rom_patch end");
	return ret;
}

static int btmtk_usb_send_hci_tci_set_sleep_cmd_7668(void)
{
	u8 cmd[] = { 0x7A, 0xFC, 0x07, 0x05, 0x40, 0x06, 0x40, 0x06, 0x00, 0x00 };
	u8 event[] = { 0x0E, 0x04, 0x01, 0x7A, 0xFC, 0x00 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_wmt_power_on_cmd_7668(void)
{
	u8 count = 0;	/* retry 3 times */
	u8 cmd[] = { 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x01 };
	u8 event[] = { 0xE4, 0x05, 0x02, 0x06, 0x01, 0x00 };	/* event[6] is key */
	int ret = -1;	/* if successful, 0 */

	BTUSB_INFO("%s: begin", __func__);
	ret = btmtk_usb_check_bt_power_status_7668();
	if (ret < 0) {
		BTUSB_ERR("%s: get bt power status failed!", __func__);
		return ret;
	} else if (ret == 1) {
		BTUSB_WARN("%s: dongle is already on, no need to send wmt power on again!", __func__);
		g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_ON;
		return 0;
	}

	g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWERING_ON;
	do {
		BTUSB_INFO("%s: begin", __func__);
		ret = btmtk_usb_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 100, 20);
		if (ret < 0) {
			BTUSB_ERR("%s: failed(%d)", __func__, ret);
			g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;

		} else if (ret == sizeof(event) + 1) {
			switch (g_data->io_buf[6]) {
			case 0:			 /* successful */
				BTUSB_INFO("%s: OK", __func__);
				g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_ON;
				ret = 0;
				break;
			case 2:			 /* retry */
				BTUSB_INFO("%s: Try again", __func__);
				msleep(100);
				continue;
			default:
				BTUSB_WARN("%s: Unknown result: %02X", __func__, g_data->io_buf[6]);
				g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;
				return -1;
			}

		} else {
			BTUSB_ERR("%s: failed, incorrect response length(%d)", __func__, ret);
			g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;
			return -1;
		}
	} while (++count < 5 && ret > 0);

	return ret;
}

static int btmtk_usb_send_wmt_power_off_cmd_7668(void)
{
	u8 cmd[] = { 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x00 };
	u8 event[] = { 0xE4, 0x05, 0x02, 0x06, 0x01, 0x00, 0x00 };
	int ret = -1;	/* if successful, 0 */

	if (g_data->is_mt7668_dongle_state == BTMTK_USB_7668_DONGLE_STATE_POWER_OFF) {
		BTUSB_ERR("%s: mt7668_dongle_state already power off", __func__);
		return 0;
	}

	ret = btmtk_usb_check_bt_power_status_7668();
	if (ret < 0) {
		BTUSB_ERR("%s: get bt power status failed!", __func__);
		return ret;
	} else if (ret == 0) {
		BTUSB_WARN("%s: dongle is already off, no need to send wmt power off again!", __func__);
		return 0;
	}

	BTUSB_INFO("%s: begin", __func__);
	g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWERING_OFF;
	ret = btmtk_usb_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 20, 0);
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
		g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_ERROR;
		return ret;
	}

	BTUSB_INFO("%s: OK", __func__);
	g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_OFF;
	return 0;
}
#endif /* SUPPORT_MT7668 */

static void btmtk_usb_cap_init(void)
{
#if SUPPORT_MT7662
	btmtk_usb_switch_iobase_7662(WLAN);
	btmtk_usb_io_read32_7662(0x00, &g_data->chip_id);

	if (g_data->chip_id) {
		BTUSB_INFO("%s: chip id = %x", __func__, g_data->chip_id);
		if (is_mt7662T(g_data)) {
			BTUSB_INFO("%s: This is MT7662T chip", __func__);
			if (g_data->rom_patch_bin_file_name) {
				memset(g_data->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);
				memcpy(g_data->rom_patch_bin_file_name, "mt7662t_patch_e1_hdr.bin", 24);
			}
			g_data->rom_patch_offset = 0xBC000;
			g_data->rom_patch_len = 0;
			return;
		} else if (is_mt7662(g_data)) {
			BTUSB_INFO("%s: This is MT7662U chip", __func__);
			if (g_data->rom_patch_bin_file_name) {
				memset(g_data->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);
				memcpy(g_data->rom_patch_bin_file_name, "mt7662_patch_e3_hdr.bin", 23);
			}
			g_data->rom_patch_offset = 0x90000;
			g_data->rom_patch_len = 0;
			return;
		}
	}
#endif /* SUPPORT_MT7662 */

#if SUPPORT_MT7668
	btmtk_usb_io_read32_7668(0x80000008, &g_data->chip_id);
	if (g_data->chip_id) {
		BTUSB_INFO("%s: chip id = %x", __func__, g_data->chip_id);
		if (is_mt7668(g_data)) {
			unsigned int fw_version = 0;

			memcpy(g_data->woble_setting_file_name,
				WOBLE_SETTING_FILE_NAME,
				sizeof(WOBLE_SETTING_FILE_NAME));
			BTUSB_INFO("%s: woble setting file name is %s", __func__, WOBLE_SETTING_FILE_NAME);

			btmtk_usb_io_read32_7668(0x80000004, &fw_version);
			memset(g_data->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);
			if (fw_version == 0x8a00) {
				BTUSB_INFO("%s: This is MT7668 E1 chip", __func__);
				memcpy(g_data->rom_patch_bin_file_name, "mt7668_patch_e1_hdr.bin", 23);
			} else if (fw_version == 0x8b01) {
				BTUSB_INFO("%s: This is MT7668 E2 chip", __func__);
				memcpy(g_data->rom_patch_bin_file_name, "mt7668_patch_e2_hdr.bin", 23);
			} else
				BTUSB_ERR("%s: Unknown MT7668 fw version 0x%x", __func__, fw_version);

			g_data->rom_patch_offset = 0x2000000;
			g_data->rom_patch_len = 0;
			return;
		}
	}
#endif /* SUPPORT_MT7668 */
#if SUPPORT_MT7663
	btmtk_usb_io_read32_7668(0x80000008, &g_data->chip_id);
	if (g_data->chip_id) {
		BTUSB_INFO("%s: chip id = %x", __func__, g_data->chip_id);
		if (is_mt7663(g_data)) {
			unsigned int fw_version = 0;

			memcpy(g_data->woble_setting_file_name,
				WOBLE_SETTING_FILE_NAME,
				sizeof(WOBLE_SETTING_FILE_NAME));
			BTUSB_INFO("%s: woble setting file name is %s", __func__, WOBLE_SETTING_FILE_NAME);

			btmtk_usb_io_read32_7668(0x80000004, &fw_version);
			memset(g_data->rom_patch_bin_file_name, 0, MAX_BIN_FILE_NAME_LEN);
			if (fw_version == 0x8a00) {
				BTUSB_INFO("%s: This is MT7663 E1 chip", __func__);
				memcpy(g_data->rom_patch_bin_file_name, "mt7663_patch_e1_hdr.bin", 23);
			} else if (fw_version == 0x8a01) {
				BTUSB_INFO("%s: This is MT7663 E2 chip", __func__);
				memcpy(g_data->rom_patch_bin_file_name, "mt7663_patch_e2_hdr.bin", 23);
			} else {
				memcpy(g_data->rom_patch_bin_file_name, "mt7663_patch_e1_hdr.bin", 23);
				BTUSB_ERR("%s: Unknown MT7663 fw version 0x%x", __func__, fw_version);
			}

			g_data->rom_patch_offset = 0x2000000;
			g_data->rom_patch_len = 0;
			return;
		}
	}
#endif /* SUPPORT_MT7663 */

	BTUSB_WARN("Unknown chip(%x)", g_data->chip_id);
}

static int btmtk_usb_load_rom_patch(void)
{
	int err = -1;

	if (g_data == NULL) {
		BTUSB_ERR("%s: g_data is NULL !", __func__);
		return err;
	}

#if SUPPORT_MT7662
	if (is_mt7662(g_data))
		return btmtk_usb_load_rom_patch_7662();
#endif

#if SUPPORT_MT7668
	if (is_mt7668(g_data))
		return btmtk_usb_load_rom_patch_7668();
#endif

#if SUPPORT_MT7663
	if (is_mt7663(g_data))
		return btmtk_usb_load_rom_patch_7668();
#endif

	BTUSB_WARN("%s: unknown chip id (%d)", __func__, g_data->chip_id);
	return err;
}

/*============================================================================*/
/* Internal Functions : Send HCI/WMT */
/*============================================================================*/
#define ______________________________________Internal_Functions_Send_HCI_WMT
static int btmtk_usb_send_woble_suspend_cmd(void)
{
	int ret = 0;	/* if successful, 0 */
#if SUPPORT_LEGACY_WOBLE
	if (need_reset_stack == HW_ERR_NONE) {
		BTUSB_INFO("%s: set need_reset_stack = %d",
				__func__, HW_ERR_CODE_LEGACY_WOBLE);
		need_reset_stack = HW_ERR_CODE_LEGACY_WOBLE;
	}
#if BT_RC_VENDOR_DEFAULT
	do {
		u8 cmd[] = { 0xC9, 0xFC, 0x0D, 0x01, 0x0E, 0x00, 0x05, 0x43,
				0x52, 0x4B, 0x54, 0x4D, 0x20, 0x04, 0x32, 0x00 };

		BTUSB_INFO("%s: BT_RC_VENDOR_T0 or Default", __func__);
		ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), NULL, -1);
	} while (0);
#elif BT_RC_VENDOR_S0
	do {
		u8 cmd[] = { 0xC9, 0xFC, 0x02, 0x01, 0x0B };

		BTUSB_INFO("%s: BT_RC_VENDOR_S0", __func__);
		ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), NULL, -1);
	} while (0);
#endif
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}
#endif /* SUPPORT_LEGACY_WOBLE */
	return ret;
}

static int btmtk_usb_send_wmt_reset_cmd(void)
{
	u8 cmd[] = { 0x6F, 0xFC, 0x05, 0x01, 0x07, 0x01, 0x00, 0x04 };
	u8 event[] = { 0xE4, 0x05, 0x02, 0x07, 0x01, 0x00, 0x00 };
	int ret = -1;	/* if successful, 0 */

	BTUSB_INFO("%s", __func__);
	BTUSB_DBG_RAW(cmd, sizeof(cmd), "%s: Send CMD:", __func__);
	ret = btmtk_usb_send_wmt_cmd(cmd, sizeof(cmd), event, sizeof(event), 20, 0);
	if (ret < 0) {
		BTUSB_ERR("%s: Check reset wmt result: NG", __func__);
	} else {
		BTUSB_INFO("%s: Check reset wmt result: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_get_rom_patch_result(void)
{
	u8 event[] = { 0xE4, 0x05, 0x02, 0x01, 0x01, 0x00, 0x00 };
	int ret = -1;	/* if successful, 0 */

	if (g_data == NULL) {
		BTUSB_ERR("%s: g_data == NULL!", __func__);
		return -1;
	}
	if (g_data->udev == NULL) {
		BTUSB_ERR("%s: g_data->udev == NULL!", __func__);
		return -1;
	}
	if (g_data->io_buf == NULL) {
		BTUSB_ERR("%s: g_data->io_buf == NULL!", __func__);
		return -1;
	}

	memset(g_data->io_buf, 0, USB_IO_BUF_SIZE);
	ret = usb_control_msg(g_data->udev, usb_rcvctrlpipe(g_data->udev, 0),
				0x01, DEVICE_VENDOR_REQUEST_IN, 0x30, 0x00,
				g_data->io_buf, USB_IO_BUF_SIZE,
				USB_CTRL_IO_TIMO);

	if (ret < 0)
		BTUSB_ERR("%s: error(%d)", __func__, ret);

	/* ret should be 16 bytes */
	if (ret >= sizeof(event) && !memcmp(g_data->io_buf, event, sizeof(event))) {
		BTUSB_INFO("Get rom patch result: OK");
		return 0;

	} else {
		BTUSB_WARN("Get rom patch result: NG");
		BTUSB_DBG_RAW(g_data->io_buf, ret, "%s: Get unknown event is:", __func__);
		return -1;
	}

	return ret;
}

static int btmtk_usb_send_hci_reset_cmd(void)
{
	u8 cmd[] = { 0x03, 0x0C, 0x00 };
	u8 event[] = { 0x0E, 0x04, 0x01, 0x03, 0x0C, 0x00 };
	int ret = -1;	/* if successful, 0 */

	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), event, sizeof(event));
	if (ret < 0) {
		BTUSB_ERR("%s: failed(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_assert_cmd_ctrl(void)
{
	u8 cmd[] = { 0x6F, 0xFC, 0x05, 0x01, 0x02, 0x01, 0x00, 0x08 };
	int ret = -1;	/* if successful, 0 */

	BTUSB_DBG_RAW(cmd, sizeof(cmd), "%s: Send CMD:", __func__);
	/* Ask DO NOT read event */
	ret = btmtk_usb_send_hci_cmd(cmd, sizeof(cmd), NULL, -1);
	if (ret < 0) {
		BTUSB_ERR("%s: error(%d)", __func__, ret);
	} else {
		BTUSB_INFO("%s: OK", __func__);
		ret = 0;
	}

	return ret;
}

static int btmtk_usb_send_assert_cmd_bulk(void)
{
	int ret = 0;
	int actual_length = 9;
	u8 buf[] = { 0x6F, 0xFC, 0x05, 0x00, 0x01, 0x02, 0x01, 0x00, 0x08 };

	BTUSB_DBG_RAW(buf, sizeof(buf), "%s: Send CMD:", __func__);
	memcpy(g_data->o_usb_buf, buf, sizeof(buf));
	ret = usb_bulk_msg(g_data->udev, usb_sndbulkpipe(g_data->udev, 2),
			g_data->o_usb_buf, sizeof(buf), &actual_length, 100);

	if (ret < 0) {
		BTUSB_ERR("%s: error(%d)", __func__, ret);
		return ret;
	}
	BTUSB_INFO("%s: OK", __func__);
	return 0;
}


static int btmtk_usb_unify_woble_wake_up(void)
{
	int ret = -1;
	int i = 0;
	u8 event_complete[] = { 0x0e, 0x07, 0x01, 0x57, 0xfd, 0x00 };
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	BTUSB_INFO("%s: handle leave woble from file", __func__);

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not opened, return", __func__);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}
	FOPS_MUTEX_UNLOCK();

	ret = btmtk_usb_handle_leaving_WoBLE_state();
	if (ret) {
		BTUSB_ERR("%s: btmtk_usb_handle_leaving_WoBLE_state return fail %d", __func__, ret);
		goto resume_woble_done;
	}

	if (g_data->woble_setting_len) {
		if (g_data->woble_setting_apcf_resume[0].length) {
			BTUSB_INFO("%s: handle leave woble apcf from file", __func__);
			for (i = 0; i < WOBLE_SETTING_COUNT; i++) {
				if (!g_data->woble_setting_apcf_resume[i].length)
					continue;

				BTUSB_INFO_RAW(g_data->woble_setting_apcf_resume[i].content,
					g_data->woble_setting_apcf_resume[i].length,
					"%s: send radio on apcf %d:", __func__, i);

				ret = btmtk_usb_send_hci_cmd(
					g_data->woble_setting_apcf_resume[i].content,
					g_data->woble_setting_apcf_resume[i].length, NULL, -1);
				if (ret < 0) {
					BTUSB_ERR("%s: Send command fail %d", __func__, ret);
					goto resume_woble_done;
				}

				ret = btmtk_usb_wait_until_event(event_complete, sizeof(event_complete),
						WOBLE_COMP_EVENT_TIMO, WOBLE_EVENT_INTERVAL_TIMO, 0);
				if (ret < 0) {
					BTUSB_ERR("%s: btmtk_usb_wait_until_event error %d", __func__, ret);
					goto resume_woble_done;
				}
			}
		}

	} else { /* use default */
		BTUSB_WARN("%s: use default leave woble", __func__);
		ret = btmtk_usb_del_Woble_APCF_index();
		if (ret) {
			BTUSB_ERR("%s: btmtk_usb_del_Woble_APCF_index return fail %d", __func__, ret);
			goto resume_woble_done;
		}
	}
	BTUSB_INFO("%s: handle leave woble end", __func__);

resume_woble_done:
	if (ret) {
		BTUSB_INFO("%s: woble_resume_fail!!!", __func__);
		ret = WOBLE_FAIL;
	} else {
		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
		USB_MUTEX_UNLOCK();
		g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_ON;
		BTUSB_INFO("%s: success", __func__);
	}
	return ret;
}

static int btmtk_usb_unify_woble_suspend(struct btmtk_usb_data *data)
{
	int ret = -1;

	BTUSB_INFO("%s", __func__);
	ret = btmtk_usb_handle_entering_WoBLE_state();
	if (ret)
		BTUSB_ERR("%s: suspend_woble_done may error!!!", __func__);

	return ret;
}

void btmtk_usb_trigger_core_dump(void)
{
	int state = BTMTK_USB_STATE_UNKNOWN;

	BTUSB_WARN("%s: Invoked by other module (WiFi).", __func__);
	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_FW_DUMP || state == BTMTK_USB_STATE_SUSPEND_FW_DUMP
			|| state == BTMTK_USB_STATE_RESUME_FW_DUMP) {
		BTUSB_WARN("%s: current in dump state, skip.", __func__);
		USB_MUTEX_UNLOCK();
		return;
	}
	USB_MUTEX_UNLOCK();

	btmtk_usb_toggle_rst_pin();
}
EXPORT_SYMBOL(btmtk_usb_trigger_core_dump);

/*============================================================================*/
/* Callback Functions */
/*============================================================================*/
#define ___________________________________________________Callback_Functions

static void btmtk_usb_load_rom_patch_complete(const struct urb *urb)
{
	struct completion *sent_to_mcu_done = (struct completion *)urb->context;

	complete(sent_to_mcu_done);
}

static void btmtk_usb_intr_complete(struct urb *urb)
{
	u8 *event_buf;
	u8 ebf0 = 0, ebf1 = 0, ebf2 = 0;
	u32 roomLeft, last_len, length;
	int err;
	static u8 intr_blocking_usb_warn;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return;
	}

	if (urb->status != 0 && intr_blocking_usb_warn < 10) {
		intr_blocking_usb_warn++;
		BTUSB_WARN("%s: urb %p urb->status %d count %d", __func__,
			urb, urb->status, urb->actual_length);
	} else if (urb->status == 0 && urb->actual_length != 0) {
		intr_blocking_usb_warn = 0;
	}

	if (urb->status == 0 && urb->actual_length != 0) {
		event_buf = urb->transfer_buffer;
		ebf0 = event_buf[0];	/* event code */
		ebf1 = event_buf[1];	/* length */
		ebf2 = event_buf[2];	/* status */

		if ((urb->actual_length == 2 + ebf1) && ebf0 == 0xFF && (ebf2 == 0x50 || ebf2 == 0x51)) {
			btmtk_usb_dispatch_event(event_buf, urb->actual_length);
			/* should drop this packet */
			goto intr_resub;
#define HCE_DIS_CONN_COMPLETE  0x05
#define HCE_SYNC_CONN_COMPLETE 0x2C
#define HCE_SYNC_CONN_COMPLETE_LEN 19
#define HCE_SYNC_CONN_COMPLETE_AIR_MODE_OFFSET 18
#define HCE_SYNC_CONN_COMPLETE_AIR_MODE_CVSD 0x02
#define HCE_SYNC_CONN_COMPLETE_AIR_MODE_TRANSPARENT 0x03
		/* For synchronous connection & disconnection */
		} else if (urb->actual_length == HCE_SYNC_CONN_COMPLETE_LEN &&
				ebf0 == HCE_SYNC_CONN_COMPLETE && ebf2 == 0x00) {
			if (sco_handle)
				BTUSB_WARN("More than ONE SCO link");

			sco_handle = event_buf[3] + (event_buf[4] << 8);
			if (event_buf[HCE_SYNC_CONN_COMPLETE_AIR_MODE_OFFSET] == HCE_SYNC_CONN_COMPLETE_AIR_MODE_CVSD)
				g_data->isoc_alt_setting = ISOC_IF_ALT_CVSD;
			else if (event_buf[18] == HCE_SYNC_CONN_COMPLETE_AIR_MODE_TRANSPARENT)
				g_data->isoc_alt_setting = ISOC_IF_ALT_MSBC;
			else
				g_data->isoc_alt_setting = ISOC_IF_ALT_DEFAULT;

			BTUSB_INFO("Synchronous Connection Complete, Handle=0x%04X, Isoc_Alt_Setting=%d",
						sco_handle, g_data->isoc_alt_setting);
		} else if (ebf0 == HCE_DIS_CONN_COMPLETE && ebf2 == 0x00) {
			if ((event_buf[3] + (event_buf[4] << 8)) == sco_handle) {
				BTUSB_INFO("Synchronous Disconnection Complete, 0x%04X", sco_handle);
				sco_handle = 0;
			}
		}

		btmtk_usb_dispatch_data_bluetooth_kpi(event_buf, urb->actual_length, HCI_EVENT_PKT);

		btmtk_usb_hci_snoop_save_event(urb->actual_length, urb->transfer_buffer);

		/* In this condition, need to add header */
		if (leftHciEventSize == 0)
			length = urb->actual_length + 1;
		else
			length = urb->actual_length;

		btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

		/* roomleft means the usable space */
		if (g_data->metabuffer->read_p <= g_data->metabuffer->write_p)
			roomLeft = META_BUFFER_SIZE - g_data->metabuffer->write_p + g_data->metabuffer->read_p - 1;
		else
			roomLeft = g_data->metabuffer->read_p - g_data->metabuffer->write_p - 1;

		/* no enough space to store the received data */
		if (roomLeft < length)
			BTUSB_WARN("%s: Queue is full !!", __func__);

		if (length + g_data->metabuffer->write_p < META_BUFFER_SIZE) {
			/* only one interrupt event, not be splited */
			if (leftHciEventSize == 0) {
				/* copy event header: 0x04 */
				g_data->metabuffer->buffer[g_data->metabuffer->write_p] = HCI_EVENT_PKT;
				g_data->metabuffer->write_p += 1;
			}
			/* copy event data */
			memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p, event_buf, urb->actual_length);
			g_data->metabuffer->write_p += urb->actual_length;
		} else {
			BTUSB_DBG("%s: back to meta buffer head", __func__);
			last_len = META_BUFFER_SIZE - g_data->metabuffer->write_p;

			/* only one interrupt event, not be splited */
			if (leftHciEventSize == 0) {
				if (last_len != 0) {
					/* copy evnet header: 0x04 */
					g_data->metabuffer->buffer[g_data->metabuffer->write_p] = HCI_EVENT_PKT;
					g_data->metabuffer->write_p += 1;
					last_len--;
					/* copy event data */
					memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
							event_buf, last_len);
					memcpy(g_data->metabuffer->buffer, event_buf + last_len,
							urb->actual_length - last_len);
					g_data->metabuffer->write_p = urb->actual_length - last_len;
				} else {
					g_data->metabuffer->buffer[0] = HCI_EVENT_PKT;
					g_data->metabuffer->write_p = 1;
					/* copy event data */
					memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
							event_buf, urb->actual_length);
					g_data->metabuffer->write_p += urb->actual_length;
				}
			/* leftHciEventSize != 0 */
			} else {
				/* copy event data */
				memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p, event_buf, last_len);
				memcpy(g_data->metabuffer->buffer, event_buf + last_len, urb->actual_length - last_len);
				g_data->metabuffer->write_p = urb->actual_length - last_len;
			}
		}

		btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

		if (leftHciEventSize != 0) {
			leftHciEventSize -= urb->actual_length;

			/* error handling. Length wrong, drop some bytes to recovery counter!! */
			if (leftHciEventSize < 0) {
				BTUSB_WARN("%s: *size wrong(%d), this event may be wrong!", __func__, leftHciEventSize);
				leftHciEventSize = 0;	/* reset count */
			}

			if (leftHciEventSize == 0) {
				if (btmtk_usb_get_state() == BTMTK_USB_STATE_WORKING) {
					wake_up(&BT_wq);
					wake_up_interruptible(&inq);
				} else {
					BTUSB_WARN("%s: current is in suspend/resume (%d), Don't wake-up wait queue",
							__func__, btmtk_usb_get_state());
				}
			}
		} else if (leftHciEventSize == 0) {
			if (btmtk_usb_get_state() == BTMTK_USB_STATE_WORKING) {
				wake_up(&BT_wq);
				wake_up_interruptible(&inq);
			} else {
				BTUSB_WARN("%s: current is in suspend/resume (%d), Don't wake-up wait queue",
						__func__, btmtk_usb_get_state());
			}
		}
	} else {
		BTUSB_INFO("%s: urb->status:%d, urb->length:%d", __func__, urb->status, urb->actual_length);
	}
intr_resub:
	usb_mark_last_busy(g_data->udev);
	usb_anchor_urb(urb, &g_data->intr_in_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed; */
		/* -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s: urb %p failed to resubmit intr_in_urb(%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	}
}

static void btmtk_usb_bulk_in_complete(struct urb *urb)
{
	/* actual_length: the ACL data size (doesn't contain header) */
	u32 roomLeft, last_len, length, index, actual_length;
	u8 *event_buf;
	u8 coredump_end = 0;
	int state = btmtk_usb_get_state();
	int err;
	u8 *buf;
	u16 len = 0;
	static u8 block_bulkin_usb_warn;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return;
	}

	roomLeft = 0;
	last_len = 0;
	length = 0;
	index = 0;
	actual_length = 0;

	if (urb->status != 0 && block_bulkin_usb_warn < 10) {
		block_bulkin_usb_warn++;
		BTUSB_INFO("%s: urb %p urb->status %d count %d", __func__, urb, urb->status, urb->actual_length);
	} else if (urb->status == 0 && urb->actual_length != 0) {
		block_bulkin_usb_warn = 0;
	}

	/* Handle FW Dump Data */
	buf = urb->transfer_buffer;
	if (urb->actual_length > 4) {
		len = buf[2] + ((buf[3] << 8) & 0xff00);
		if (buf[0] == 0x6f && buf[1] == 0xfc && len + 4 == urb->actual_length) {
			static int print_dump_data_counter;

			if (state != BTMTK_USB_STATE_FW_DUMP && state != BTMTK_USB_STATE_RESUME_FW_DUMP) {
				/* This is the first BULK_IN packet of FW dump. */
				BTUSB_INFO("btmtk_usb FW dump begin");

				if (state == BTMTK_USB_STATE_RESUME)
					btmtk_usb_set_state(BTMTK_USB_STATE_RESUME_FW_DUMP);
				else if (state == BTMTK_USB_STATE_SUSPEND)
					btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND_FW_DUMP);
				else
					btmtk_usb_set_state(BTMTK_USB_STATE_FW_DUMP);

				/* Print too much log in ISR may cause kernel panic. */
				/* btmtk_usb_hci_snoop_print_to_log(); */
				print_dump_data_counter = 0;
			}

			/* print dump data to console */
			if (print_dump_data_counter < 20) {
				print_dump_data_counter++;
				BTUSB_INFO("btmtk_usb FW dump data (%d): %s", print_dump_data_counter, &buf[4]);
			}
			/* Save file by fifo */

#if ENABLE_BT_FIFO_THREAD
			btmtk_fifo_in(FIFO_COREDUMP, g_data->bt_fifo, (const void *)&buf[4], len);
#endif

			if (buf[urb->actual_length - 6] == ' ' &&
				buf[urb->actual_length - 5] == 'e' &&
				buf[urb->actual_length - 4] == 'n' &&
				buf[urb->actual_length - 3] == 'd') {
				/* This is the latest BULK_IN packet of FW dump. */
				BTUSB_INFO("btmtk_usb FW dump end");
				coredump_end = 1;
			}
			/* Add coredump data to fwlog_queue for picus tool */
			btmtk_usb_dispatch_acl(buf, urb->actual_length);

			/* set to 0xff to avoid BlueDroid dropping this ACL packet. */
			buf[0] = 0xff;
			buf[1] = 0xff;
			goto bulk_intr_resub;
		} else if (buf[0] == 0xff && buf[1] == 0x05 && len + 4 == urb->actual_length) {
			/* Picus log by ACL */
			/* Picus header : FF 05 xx xx */
			btmtk_usb_dispatch_acl(buf, urb->actual_length);
			goto bulk_intr_resub;
		}
	}

	if (urb->status == 0) {
		event_buf = urb->transfer_buffer;
		len = buf[2] + ((buf[3] << 8) & 0xff00);

		btmtk_usb_dispatch_data_bluetooth_kpi(event_buf, urb->actual_length, HCI_ACLDATA_PKT);

		btmtk_usb_hci_snoop_save_acl(urb->actual_length, urb->transfer_buffer);

		if (urb->actual_length > 4 && event_buf[0] == 0x6f
			&& event_buf[1] == 0xfc && len + 4 == urb->actual_length) {
			BTUSB_DBG("Coredump message");
		} else {
			length = urb->actual_length + 1;

			actual_length =
				1 * (event_buf[2] & 0x0f) +
				16 * ((event_buf[2] & 0xf0) >> 4)
				+ 256 * ((event_buf[3] & 0x0f)) +
				4096 * ((event_buf[3] & 0xf0) >> 4);

			btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

			/* roomleft means the usable space */
			if (g_data->metabuffer->read_p <=
				g_data->metabuffer->write_p)
				roomLeft = META_BUFFER_SIZE - g_data->metabuffer->write_p +
						g_data->metabuffer->read_p - 1;
			else
				roomLeft = g_data->metabuffer->read_p - g_data->metabuffer->write_p - 1;

			/* no enough space to store the received data */
			if (roomLeft < length)
				BTUSB_WARN("%s: Queue is full !!", __func__);

			if (length + g_data->metabuffer->write_p <
				META_BUFFER_SIZE) {

				if (leftACLSize == 0) {
					/* copy ACL data header: 0x02 */
					g_data->metabuffer->buffer[g_data->metabuffer->write_p] = HCI_ACLDATA_PKT;
					g_data->metabuffer->write_p += 1;
				}

				/* copy event data */
				memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
						event_buf, urb->actual_length);
				g_data->metabuffer->write_p += urb->actual_length;
			} else {
				last_len = META_BUFFER_SIZE - g_data->metabuffer->write_p;
				if (leftACLSize == 0) {
					if (last_len != 0) {
						/* copy ACL data header: 0x02 */
						g_data->metabuffer->buffer[g_data->metabuffer->write_p]
							= HCI_ACLDATA_PKT;
						g_data->metabuffer->write_p += 1;
						last_len--;
						/* copy event data */
						memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
								event_buf, last_len);
						memcpy(g_data->metabuffer->buffer, event_buf + last_len,
								urb->actual_length - last_len);
						g_data->metabuffer->write_p = urb->actual_length - last_len;
					} else {
						g_data->metabuffer->buffer[0] = HCI_ACLDATA_PKT;
						g_data->metabuffer->write_p = 1;
						/* copy event data */
						memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
								event_buf, urb->actual_length);
						g_data->metabuffer->write_p += urb->actual_length;
					}
				} else {	/* leftACLSize !=0 */

					/* copy event data */
					memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p,
							event_buf, last_len);
					memcpy(g_data->metabuffer->buffer, event_buf + last_len,
							urb->actual_length - last_len);
					g_data->metabuffer->write_p = urb->actual_length - last_len;
				}
			}

			btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

			/* the maximize bulk in ACL data packet size is 512 (4byte header + 508 byte data) */
			/* maximum receieved data size of one packet is 1025 (4byte header + 1021 byte data) */
			if (leftACLSize == 0 && actual_length > 1021) {
				/* the data in next interrupt event */
				leftACLSize = actual_length + 4 - urb->actual_length;
				BTUSB_ERR("ERROR !!! too large ACL data length %d", leftACLSize);
			} else if (leftACLSize > 0) {
				leftACLSize -= urb->actual_length;

				/* error handling. Length wrong, drop some bytes to recovery counter!! */
				if (leftACLSize < 0) {
					BTUSB_WARN("* size wrong(%d), this acl data may be wrong!!", leftACLSize);
					leftACLSize = 0;	/* reset count */
				}

				if (leftACLSize == 0) {
					if (state == BTMTK_USB_STATE_WORKING || state == BTMTK_USB_STATE_FW_DUMP ||
						state == BTMTK_USB_STATE_RESUME_FW_DUMP) {
						wake_up(&BT_wq);
						wake_up_interruptible(&inq);
					} else {
						BTUSB_DBG("%s: now is in suspend/resume(%d), Don't wake-up wait queue",
								__func__, state);
					}
				}
			} else if (leftACLSize == 0 && actual_length <= 1021) {
				if (state == BTMTK_USB_STATE_WORKING || state == BTMTK_USB_STATE_FW_DUMP ||
					state == BTMTK_USB_STATE_RESUME_FW_DUMP) {
					wake_up(&BT_wq);
					wake_up_interruptible(&inq);
				} else {
					BTUSB_DBG("%s: current is in suspend/resume (%d), Don't wake-up wait queue",
							__func__, state);
				}
			} else {
				BTUSB_WARN("ACL data count fail, leftACLSize:%d", leftACLSize);
			}
		}
	} else {
		BTUSB_INFO("%s: urb->status:%d", __func__, urb->status);
	}
bulk_intr_resub:
	if (coredump_end) {
		if (need_reset_stack == HW_ERR_NONE)
			need_reset_stack = HW_ERR_CODE_CORE_DUMP;
#if !ENABLE_BT_FIFO_THREAD
		btmtk_usb_toggle_rst_pin();
#endif
	}

	usb_anchor_urb(urb, &g_data->bulk_in_anchor);
	usb_mark_last_busy(g_data->udev);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err != 0) {
		/* -EPERM: urb is being killed; */
		/* -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s: urb %p failed to resubmit bulk_in_urb(%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	}
}

static void btmtk_usb_tx_complete_meta(const struct urb *urb)
{
	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return;
	}

	usb_free_coherent(g_data->udev, urb->transfer_buffer_length, urb->transfer_buffer, urb->transfer_dma);

	g_data->meta_tx = 0;
	kfree(urb->setup_packet);
}

static int btmtk_usb_send_apcf_reserved(void)
{
	int ret = -1;
	u8 reserve_apcf_cmd_7668[] = { 0x5C, 0xFC, 0x01, 0x0A };
	u8 reserve_apcf_event_7668[] = { 0x0e, 0x06, 0x01, 0x5C, 0xFC, 0x00 };

	u8 reserve_apcf_cmd_7663[] = { 0x85, 0xFC, 0x01, 0x0A };
	u8 reserve_apcf_event_7663[] = { 0x0e, 0x06, 0x01, 0x85, 0xFC, 0x00, 0x0A, 0x08};

	if (is_mt7668(g_data))
		ret = btmtk_usb_send_hci_cmd(reserve_apcf_cmd_7668, sizeof(reserve_apcf_cmd_7668),
			reserve_apcf_event_7668, sizeof(reserve_apcf_event_7668));
	else if (is_mt7663(g_data))
		ret = btmtk_usb_send_hci_cmd(reserve_apcf_cmd_7663, sizeof(reserve_apcf_cmd_7663),
			reserve_apcf_event_7663, sizeof(reserve_apcf_event_7663));
	else
		BTUSB_WARN("%s: not support for 0x%x", __func__, g_data->chip_id);
	if (ret > 0)
		ret = 0;
	else
		BTUSB_ERR("%s: btmtk_usb_send_hci_cmd return error ret %d", __func__, ret);

	BTUSB_INFO("%s: ret %d", __func__, ret);
	return ret;
}

static int btmtk_usb_send_get_vendor_cap(void)
{
	int ret = -1;
	u8 get_vendor_cap_cmd[] = { 0x53, 0xFD, 0x00 };
	u8 get_vendor_cap_event[] = { 0x0e, 0x12, 0x01, 0x53, 0xFD, 0x00,
		/* 0x64, 0x01, 0xb0, 0x4f, 0x32, 0x01 */ };

	ret = btmtk_usb_send_hci_cmd(get_vendor_cap_cmd, sizeof(get_vendor_cap_cmd),
		get_vendor_cap_event, sizeof(get_vendor_cap_event));
	if (ret > 0)
		ret = 0;
	else
		BTUSB_ERR("%s: btmtk_usb_send_hci_cmd return error ret %d", __func__, ret);

	BTUSB_INFO("%s: ret %d", __func__, ret);
	return ret;
}

static int btmtk_usb_standby(void)
{
	int ret = -1;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	BTUSB_INFO("%s: begin", __func__);
	if (is_support_unify_woble(g_data)) {
		ret = btmtk_usb_reset_power_on();
		if (ret < 0)
			return ret;
		ret = btmtk_usb_send_apcf_reserved();
		if (ret < 0)
			return ret;
		ret = btmtk_usb_send_get_vendor_cap();
		if (ret < 0)
			return ret;
		ret = btmtk_usb_unify_woble_suspend(g_data);
		if (ret < 0)
			return ret;
	} else {
		ret = btmtk_usb_handle_entering_WoBLE_state();
		if (ret < 0)
			return ret;
	}

	BTUSB_INFO("%s: End after 500ms delay", __func__);
	msleep(500); /* Add 500ms delay to avoid log lost. */
	return 0;
}

/*============================================================================*/
/* Internal Functions : SCO and Isochronous Related */
/*============================================================================*/
#define ___________________________________Internal_Functions_SCO_Isochronous
static int btmtk_usb_set_isoc_interface(bool close)
{
	int ret = -1, i = 0;
	struct usb_endpoint_descriptor *ep_desc;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENXIO;
	}

	if ((sco_handle && g_data->isoc_urb_submitted && close == false)
			|| (sco_handle == 0 && g_data->isoc_urb_submitted == 0)) {
		return 0;

	} else if (sco_handle && g_data->isoc_urb_submitted == 0) {
		/* Alternate setting */
		ret = usb_set_interface(g_data->udev, 1, g_data->isoc_alt_setting);
		if (ret < 0) {
			BTUSB_ERR("%s: Set ISOC alternate(%d) fail", __func__, g_data->isoc_alt_setting);
			return ret;
		}
		BTUSB_INFO("%s: Set alternate to %d", __func__, g_data->isoc_alt_setting);

		for (i = 0; i < g_data->isoc->cur_altsetting->desc.bNumEndpoints; i++) {
			ep_desc = &g_data->isoc->cur_altsetting->endpoint[i].desc;

			if (usb_endpoint_is_isoc_out(ep_desc)) {
				g_data->isoc_tx_ep = ep_desc;
				BTUSB_INFO("iso_out: length: %d, addr: 0x%02X, maxSize: %d, interval: %d",
						ep_desc->bLength, ep_desc->bEndpointAddress,
						ep_desc->wMaxPacketSize, ep_desc->bInterval);
				continue;
			}

			if (usb_endpoint_is_isoc_in(ep_desc)) {
				g_data->isoc_rx_ep = ep_desc;
				BTUSB_INFO("iso_in: length: %d, addr: 0x%02X, maxSize: %d, interval: %d",
						ep_desc->bLength, ep_desc->bEndpointAddress,
						ep_desc->wMaxPacketSize, ep_desc->bInterval);
				continue;
			}
		}
		if (!g_data->isoc_tx_ep || !g_data->isoc_rx_ep) {
			BTUSB_ERR("Invalid SCO descriptors");
			return -ENODEV;
		}

		ret = btmtk_usb_submit_isoc_urb();
		if (ret < 0)
			return ret;
		BTUSB_INFO("%s: Start isoc_in.", __func__);

	} else if ((sco_handle == 0 || close == true) && g_data->isoc_urb_submitted) {
		u8 count = 0;

		while (atomic_read(&g_data->isoc_out_count) && ++count <= RETRY_TIMES) {
			BTUSB_INFO("There are isoc out packet remaining: %d ",
					atomic_read(&g_data->isoc_out_count));
			mdelay(10);
		}
		usb_kill_anchored_urbs(&g_data->isoc_in_anchor);
		g_data->isoc_urb_submitted = 0;
		BTUSB_INFO("%s: Stop isoc_in.", __func__);

		ret = usb_set_interface(g_data->udev, 1, 0);
		if (ret < 0) {
			BTUSB_ERR("%s: Set ISOC alternate(0) fail", __func__);
			return ret;
		}
		BTUSB_INFO("%s: Set alternate to 0", __func__);

	} else {
		BTUSB_INFO("%s: sco: 0x%04X, isoc_urb_submitted: %d",
				__func__, sco_handle, g_data->isoc_urb_submitted);
	}
	return 0;
}

static int btmtk_usb_submit_isoc_urb(void)
{
	struct urb *urb;
	u8 *buf;
	unsigned int pipe;
	int err, size;

	BTUSB_INFO("%s", __func__);

	if (g_data->isoc_urb_submitted) {
		BTUSB_WARN("%s: already submitted", __func__);
		return 0;
	}
	g_data->isoc_urb_submitted = 0;

	if (!g_data->isoc_rx_ep) {
		BTUSB_ERR("%s: error 1", __func__);
		return -ENODEV;
	}

	urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, GFP_KERNEL);
	if (!urb) {
		BTUSB_ERR("%s: error 2", __func__);
		return -ENOMEM;
	}

	size = le16_to_cpu(g_data->isoc_rx_ep->wMaxPacketSize) *
		BTUSB_MAX_ISOC_FRAMES;

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf) {
		usb_free_urb(urb);
		BTUSB_ERR("%s: error 3", __func__);
		return -ENOMEM;
	}

	/* For isoc in URB */
	pipe = usb_rcvisocpipe(g_data->udev, g_data->isoc_rx_ep->bEndpointAddress);

	usb_fill_int_urb(urb, g_data->udev, pipe, buf, size, (usb_complete_t)btmtk_usb_isoc_complete,
			(void *)g_data, g_data->isoc_rx_ep->bInterval);

	urb->transfer_flags = URB_FREE_BUFFER | URB_ISO_ASAP;

	__fill_isoc_descriptor(urb, size,
			le16_to_cpu(g_data->isoc_rx_ep->wMaxPacketSize));

	usb_anchor_urb(urb, &g_data->isoc_in_anchor);

	err = usb_submit_urb(urb, GFP_KERNEL);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s urb %p submission failed (%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	} else {
		g_data->isoc_urb_submitted = 1;
	}

	usb_free_urb(urb);
	return err;
}

static void btmtk_usb_isoc_complete(struct urb *urb)
{
	int err;
	struct sk_buff *skb_isoc = NULL;
	unsigned long flags = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return;
	}

	BTUSB_DBG("%s Start(status: %d, len: %d)", __func__, urb->status, urb->actual_length);

	/* Controller reply nothing to ISO_IN. */
	if (urb->status == -EPROTO) {
		goto isoc_resub;
	/* Controller reply something to ISO_IN. */
	} else if (urb->status == -EINPROGRESS || urb->status == 0) {
		if (urb->actual_length != 0 && sco_handle > 0) {
			skb_isoc = alloc_skb(urb->actual_length, GFP_ATOMIC);
			if (skb_isoc == NULL) {
				BTUSB_ERR("%s: alloc_skb return 0, error", __func__);
				goto isoc_resub;
			}

			/* queue */
			memset(skb_isoc->data, 0, urb->actual_length);
			memcpy(skb_isoc->data, urb->transfer_buffer, urb->actual_length);
			/** This print will block ISOC_IN
			 * BTUSB_DBG_RAW(skb_isoc->data, urb->actual_length,
					"%s: (len=%2d)", __func__, urb->actual_length);
			 */
			skb_isoc->len = urb->actual_length;
			ISOC_SPIN_LOCK(flags);
			skb_queue_tail(&g_data->isoc_in_queue, skb_isoc);
			ISOC_SPIN_UNLOCK(flags);
			wake_up(&BT_sco_wq);
			wake_up_interruptible(&inq_isoc);
		}
	} else {
		BTUSB_WARN("%s WARNING! status:%d", __func__, urb->status);
	}

isoc_resub:
	usb_mark_last_busy(g_data->udev);
	usb_anchor_urb(urb, &g_data->isoc_in_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected
		 */
		if (err != -EPERM && err != -ENODEV)
			BTUSB_ERR("%s urb %p failed to resubmit (%d)", __func__, urb, -err);
		usb_unanchor_urb(urb);
	}
	BTUSB_DBG("%s End", __func__);
}

static void btmtk_usb_isoc_tx_complete(const struct urb *urb)
{
	if (urb->status != 0 || urb->actual_length == 0)
		BTUSB_DBG("%s urb %p status %d count %d", __func__,
				urb, urb->status, urb->actual_length);

	kfree(urb->transfer_buffer);
	kfree(urb->setup_packet);
	if (atomic_read(&g_data->isoc_out_count))
		atomic_dec(&g_data->isoc_out_count);
}

/*============================================================================*/
/* Interface Functions : BT Stack */
/*============================================================================*/
#define ______________________________________Interface_Function_for_BT_Stack
static ssize_t btmtk_usb_fops_write(struct file *file, const char __user *buf,
					size_t count, loff_t *f_pos)
{
	int retval = 0;
	static u8 waiting_for_hci_without_packet_type; /* INITIALISED_STATIC: do not initialise statics to 0 */
	static u8 hci_packet_type = 0xff;
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_DBG("%s: current is in suspend/resume (%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		msleep(3000);
		return -EAGAIN;
	}
	USB_MUTEX_UNLOCK();

	if (need_reopen) {
		BTUSB_WARN("%s: need_reopen (%d)!", __func__, need_reopen);
		return -EFAULT;
	}

	if (g_data->is_mt7668_dongle_state == BTMTK_USB_7668_DONGLE_STATE_POWER_OFF) {
		BTUSB_WARN("%s: dongle state already power off, do not write", __func__);
		return -EFAULT;
	}

	/* semaphore mechanism, the waited process will sleep */
	down(&g_data->wr_mtx);

	if (waiting_for_hci_without_packet_type == 1 && count == 1) {
		BTUSB_WARN("%s: Waiting for hci_without_packet_type, but receive data count is 1!", __func__);
		BTUSB_WARN("%s: Treat this packet as packet_type", __func__);
		retval = copy_from_user(&hci_packet_type, &buf[0], 1);
		waiting_for_hci_without_packet_type = 1;
		retval = 1;
		goto OUT;
	}

	if (waiting_for_hci_without_packet_type == 0) {
		if (count == 1) {
			retval = copy_from_user(&hci_packet_type, &buf[0], 1);
			waiting_for_hci_without_packet_type = 1;
			retval = 1;
			goto OUT;
		}
	}

	if (count > 0) {
		u32 copy_size = (count < BUFFER_SIZE) ? count : BUFFER_SIZE;

		if (waiting_for_hci_without_packet_type) {
			retval = copy_from_user(&g_data->o_buf[1], &buf[0], copy_size);
			g_data->o_buf[0] = hci_packet_type;
			copy_size += 1;
		} else {
			retval = copy_from_user(&g_data->o_buf[0], &buf[0], copy_size);
		}

		if (retval) {
			retval = -EFAULT;
			BTUSB_ERR("%s: copy data from user fail", __func__);
			goto OUT;
		}

		btmtk_usb_dispatch_data_bluetooth_kpi(g_data->o_buf, copy_size, 0);

		/* command */
		if (g_data->o_buf[0] == HCI_COMMAND_PKT) {
			/* parsing commands */
			u8 fw_assert_cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x02, 0x01, 0x00, 0x08 };
			u8 reset_cmd[] = { 0x01, 0x03, 0x0C, 0x00 };
			u8 read_ver_cmd[] = { 0x01, 0x01, 0x10, 0x00 };

			if (copy_size == sizeof(fw_assert_cmd) &&
				!memcmp(g_data->o_buf, fw_assert_cmd, sizeof(fw_assert_cmd))) {
				BTUSB_INFO("%s: Donge FW Assert Triggered by BT Stack!", __func__);
				btmtk_usb_hci_snoop_print_to_log();
			} else if (copy_size == sizeof(reset_cmd) &&
					!memcmp(g_data->o_buf, reset_cmd, sizeof(reset_cmd))) {
				BTUSB_INFO("%s: got command: 0x03 0C 00 (HCI_RESET)", __func__);

			} else if (copy_size == sizeof(read_ver_cmd) &&
					!memcmp(g_data->o_buf, read_ver_cmd, sizeof(read_ver_cmd))) {
				BTUSB_INFO("%s: got command: 0x01 10 00 (READ_LOCAL_VERSION)", __func__);
			}

			btmtk_usb_hci_snoop_save_cmd(copy_size, &g_data->o_buf[0]);
			retval = btmtk_usb_meta_send_data(&g_data->o_buf[0], copy_size);

		/* ACL data */
		} else if (g_data->o_buf[0] == HCI_ACLDATA_PKT) {
			retval = btmtk_usb_send_data(&g_data->o_buf[0], copy_size);

		/* Unknown */
		} else {
			BTUSB_WARN("%s: this is unknown bt data:0x%02x", __func__, g_data->o_buf[0]);
		}

		if (waiting_for_hci_without_packet_type) {
			hci_packet_type = 0xff;
			waiting_for_hci_without_packet_type = 0;
			if (retval > 0)
				retval -= 1;
		}
	} else {
		retval = -EFAULT;
		BTUSB_ERR("%s: target packet length:%zu is not allowed, retval = %d", __func__, count, retval);
	}

OUT:
	up(&g_data->wr_mtx);
	return retval;
}

static ssize_t btmtk_usb_fops_writefwlog(struct file *filp, const char __user *buf,
					size_t count, loff_t *f_pos)
{
	int i = 0, len = 0, ret = -1;

	/* Command example : echo 01 be fc 01 05 > /dev/stpbtfwlog */
	if (count > HCI_MAX_COMMAND_BUF_SIZE) {
		BTUSB_ERR("%s: your command is larger than buffer length, count = %zd/%d",
				__func__, count, HCI_MAX_COMMAND_BUF_SIZE);
		return -ENOMEM;
	}

	memset(g_data->i_fwlog_buf, 0, HCI_MAX_COMMAND_BUF_SIZE);
	memset(g_data->o_fwlog_buf, 0, HCI_MAX_COMMAND_SIZE);
	if (copy_from_user(g_data->i_fwlog_buf, buf, count) != 0) {
		BTUSB_ERR("%s: Failed to copy data", __func__);
		return -ENODATA;
	}

	/* For log_lvl, EX: echo log_lvl=4 > /dev/stpbtfwlog */
	if (strcmp(g_data->i_fwlog_buf, "log_lvl=") >= 0) {
		u8 val = *(g_data->i_fwlog_buf + strlen("log_lvl=")) - 48;

		if (val > BTMTK_LOG_LEVEL_MAX || val <= 0) {
			BTUSB_ERR("%s: Got incorrect value for log level(%d)", __func__, val);
			return -EINVAL;
		}
		btmtk_log_lvl = val;
		BTUSB_INFO("%s: btmtk_log_lvl = %d", __func__, btmtk_log_lvl);
		return count;
	} else if (strcmp(g_data->i_fwlog_buf, "bperf=") >= 0) {
		u8 val = *(g_data->i_fwlog_buf + strlen("bperf=")) - 48;

		btmtk_bluetooth_kpi = val;
		BTUSB_INFO("%s: set bluetooth KPI feature(bperf) to %d", __func__, btmtk_bluetooth_kpi);
		return count;
	}

	if ((is_mt7668(g_data) || is_mt7663(g_data))
			&& g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_POWER_ON) {
		BTUSB_ERR("%s: 7668 is not opening(%d)", __func__, g_data->is_mt7668_dongle_state);
		return -EBADFD;
	}
	/* hci input command format : echo 01 be fc 01 05 > /dev/stpbtfwlog */
	/* We take the data from index three to end. */
	for (i = 0; i < count; i++) {
		char *pos = g_data->i_fwlog_buf + i;
		char temp_str[3] = {'\0'};
		long res = 0;

		if (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') {
			continue;
		} else if (*pos == '0' && (*(pos + 1) == 'x' || *(pos + 1) == 'X')) {
			i++;
			continue;
		} else if (!(*pos >= '0' && *pos <= '9') && !(*pos >= 'A' && *pos <= 'F')
			&& !(*pos >= 'a' && *pos <= 'f')) {
			BTUSB_ERR("%s: There is an invalid input(%c)", __func__, *pos);
			return -EINVAL;
		}
		temp_str[0] = *pos;
		temp_str[1] = *(pos + 1);
		i++;
		ret = kstrtol(temp_str, 16, &res);
		if (ret == 0)
			g_data->o_fwlog_buf[len++] = (u8)res;
		else
			BTUSB_ERR("%s: Convert %s failed(%d)", __func__, temp_str, ret);
	}

	/* Receive command from stpbtfwlog, then Sent hci command to controller */
	BTUSB_DBG_RAW(g_data->o_fwlog_buf, len, "%s: Input is:", __func__);

	if (g_data->o_fwlog_buf[0] != HCI_COMMAND_PKT) {
		BTUSB_ERR("%s: Not support 0x%02X yet", __func__, g_data->o_fwlog_buf[0]);
		return -EPROTONOSUPPORT;
	}
	/* check HCI command length */
	if (len > HCI_MAX_COMMAND_SIZE) {
		BTUSB_ERR("%s: command is larger than max buf size, length = %d", __func__, len);
		return -ENOMEM;
	}

	/* send HCI command */
	ret = usb_control_msg(g_data->udev, usb_sndctrlpipe(g_data->udev, 0),
				0, DEVICE_CLASS_REQUEST_OUT, 0, 0,
				(void *)g_data->o_fwlog_buf + 1, len, USB_CTRL_IO_TIMO);
	if (ret < 0) {
		BTUSB_ERR("%s: command send failed(%d)", __func__, ret);
		return -EIO;
	}
	BTUSB_INFO("%s: Write end(len: %d)", __func__, len);
	return count;	/* If input is correct should return the same length */
}

static ssize_t btmtk_usb_fops_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
	int retval = 0;
	int copyLen = 0;
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;
	unsigned short tailLen = 0;
	u8 *buffer = NULL;
	u8 hwerr_event[] = { 0x04, 0x10, 0x01, 0xff };

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}
	buffer = g_data->i_buf;

	down(&g_data->rd_mtx);

	if (count > BUFFER_SIZE)
		count = BUFFER_SIZE;

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_SUSPEND_FW_DUMP) {
		BTUSB_ERR("%s: current is BTMTK_USB_STATE_SUSPEND_FW_DUMP (%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		copyLen = -EAGAIN;
		goto OUT;
	} else if (need_reset_stack) {
		BTUSB_WARN("%s: Reset BT stack", __func__);
		USB_MUTEX_UNLOCK();

		btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
		hwerr_event[3] = need_reset_stack;
		need_reset_stack = HW_ERR_NONE;
		/* fill hwerr in tail */
		memcpy(g_data->metabuffer->buffer + g_data->metabuffer->write_p, hwerr_event, sizeof(hwerr_event));
		g_data->metabuffer->write_p += sizeof(hwerr_event);
		btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
		need_reopen = 1;
	} else if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_ERR("%s: current is in suspend/resume (%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		copyLen = -EAGAIN;
		goto OUT;
	}
	USB_MUTEX_UNLOCK();

	btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	/* means the buffer is empty */
	while (g_data->metabuffer->read_p == g_data->metabuffer->write_p) {

		/* unlock the buffer to let other process write data to buffer */
		btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

		/* If nonblocking mode, return directly O_NONBLOCK is specified during open() */
		if (file->f_flags & O_NONBLOCK) {
			/* BTUSB_DBG("Non-blocking btmtk_usb_fops_read()"); */
			retval = -EAGAIN;
			goto OUT;
		}
		wait_event(BT_wq, g_data->metabuffer->read_p != g_data->metabuffer->write_p);
		btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	}

	while (g_data->metabuffer->read_p != g_data->metabuffer->write_p) {
		if (g_data->metabuffer->write_p > g_data->metabuffer->read_p) {
			copyLen = g_data->metabuffer->write_p - g_data->metabuffer->read_p;
			if (copyLen > count)
				copyLen = count;
			memcpy(g_data->i_buf, g_data->metabuffer->buffer + g_data->metabuffer->read_p, copyLen);
			g_data->metabuffer->read_p += copyLen;
			break;

		} else {
			tailLen = META_BUFFER_SIZE - g_data->metabuffer->read_p;
			if (tailLen > count) {	/* exclude equal case to skip wrap check */
				copyLen = count;
				memcpy(g_data->i_buf, g_data->metabuffer->buffer + g_data->metabuffer->read_p, copyLen);
				g_data->metabuffer->read_p += copyLen;
			} else {
				/* part 1: copy tailLen */
				memcpy(g_data->i_buf, g_data->metabuffer->buffer + g_data->metabuffer->read_p, tailLen);

				buffer += tailLen;	/* update buffer offset */

				/* part 2: check if head length is enough */
				copyLen = count - tailLen;

				/* if write_p < copyLen: means we can copy all data until write_p; */
				/* else: we can only copy data for copyLen */
				copyLen = (g_data->metabuffer->write_p < copyLen) ? g_data->
					metabuffer->write_p : copyLen;

				/* if copylen not 0, copy data to buffer */
				if (copyLen)
					memcpy(buffer, g_data->metabuffer->buffer + 0, copyLen);
				/* Update read_p final position */
				g_data->metabuffer->read_p = copyLen;

				/* update return length: head + tail */
				copyLen += tailLen;
			}
			break;
		}
	}

	btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	/* BTUSB_DBG_RAW(g_data->i_buf, 16, "%s:  (len=%2d)", __func__, copyLen); */
	if (copy_to_user(buf, g_data->i_buf, copyLen)) {
		BTUSB_ERR("copy to user fail");
		copyLen = -EFAULT;
		goto OUT;
	}
OUT:
	up(&g_data->rd_mtx);
	return copyLen;
}

static int btmtk_usb_send_init_cmds(void)
{
#if SUPPORT_MT7662
	if (is_mt7662(g_data)) {
		btmtk_usb_send_hci_reset_cmd();
		btmtk_usb_send_hci_low_power_cmd_7662(TRUE);
		btmtk_usb_send_hci_set_ce_cmd_7662();
		btmtk_usb_send_hci_set_tx_power_cmd_7662();
		btmtk_usb_send_hci_radio_on_cmd_7662();
	}
#endif

#if SUPPORT_MT7668 || SUPPORT_MT7663
	if (is_mt7668(g_data) || is_mt7663(g_data)) {
		btmtk_usb_send_wmt_power_on_cmd_7668();
		if (g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_POWER_ON) {
			BTUSB_ERR("Power on MT7668 failed, reset it");
			if (need_reset_stack == HW_ERR_NONE)
				need_reset_stack = HW_ERR_CODE_POWER_ON;
			btmtk_usb_toggle_rst_pin();
			return -1;
		}
		if (btmtk_usb_send_hci_tci_set_sleep_cmd_7668() < 0) {
			if (need_reset_stack == HW_ERR_NONE)
				need_reset_stack = HW_ERR_CODE_SET_SLEEP_CMD;
			btmtk_usb_toggle_rst_pin();
			return -1;
		}
	}
#endif
	return 0;
}

static int btmtk_usb_send_deinit_cmds(void)
{
#if SUPPORT_MT7662
	if (is_mt7662(g_data))
		btmtk_usb_send_hci_radio_off_cmd_7662();
#endif

#if SUPPORT_MT7668 || SUPPORT_MT7663
	if (is_mt7668(g_data) || is_mt7663(g_data)) {
		btmtk_usb_send_wmt_power_off_cmd_7668();
		if (g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_POWER_OFF) {
			BTUSB_ERR("Power off MT7668 failed, reset it");
			if (need_reset_stack == HW_ERR_NONE)
				need_reset_stack = HW_ERR_CODE_POWER_OFF;
			btmtk_usb_toggle_rst_pin();
			return -1;
		}
	}
#endif
	return 0;
}

static int btmtk_usb_fops_open(struct inode *inode, struct file *file)
{
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_INIT || state == BTMTK_USB_STATE_DISCONNECT) {
		USB_MUTEX_UNLOCK();
		return -EAGAIN;
	}
	USB_MUTEX_UNLOCK();

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate == BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops opened!", __func__);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}

	if (fstate == BTMTK_FOPS_STATE_CLOSING) {
		BTUSB_WARN("%s: fops close is on-going !", __func__);
		FOPS_MUTEX_UNLOCK();
		return -EAGAIN;
	}
	FOPS_MUTEX_UNLOCK();

	BTUSB_INFO("%s: Mediatek Bluetooth USB driver ver %s", __func__, VERSION);
	BTUSB_INFO("%s: major %d minor %d (pid %d), probe counter: %d",
			__func__, imajor(inode), iminor(inode), current->pid, probe_counter);

	if (current->pid == 1) {
		BTUSB_WARN("%s: return 0", __func__);
		return 0;
	}

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_WARN("%s: not in working state(%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		return -ENODEV;
	}
	USB_MUTEX_UNLOCK();

	/* init meta buffer */
	spin_lock_init(&(g_data->metabuffer->spin_lock.lock));

	sema_init(&g_data->wr_mtx, 1);
	sema_init(&g_data->rd_mtx, 1);
	sema_init(&g_data->isoc_wr_mtx, 1);
	sema_init(&g_data->isoc_rd_mtx, 1);

	/* init wait queue */
	init_waitqueue_head(&(inq));

	/* Init Hci Snoop */
	btmtk_usb_hci_snoop_init();

	if (btmtk_usb_send_init_cmds()) {
		msleep(RESET_PIN_SET_LOW_TIME + 10);	/* wait chip reset */
		return -EAGAIN;
	}

	if (is_support_unify_woble(g_data))
		btmtk_usb_send_apcf_reserved();

	btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;
	btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	BTUSB_INFO("enable interrupt and bulk in urb");
	if (btmtk_usb_start_traffic()) {
		BTUSB_ERR("Submit interrupt failed");
		return -EAGAIN;
	}

	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_OPENED);
	FOPS_MUTEX_UNLOCK();
	need_reopen = 0;
	BTUSB_INFO("%s: OK", __func__);

	return 0;
}

static int btmtk_usb_fops_close(struct inode *inode, struct file *file)
{
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not allow close(%d)", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}
	btmtk_fops_set_state(BTMTK_FOPS_STATE_CLOSING);
	FOPS_MUTEX_UNLOCK();
	BTUSB_INFO("%s: major %d minor %d (pid %d), probe:%d", __func__,
			imajor(inode), iminor(inode), current->pid, probe_counter);

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_WARN("%s: not in working state(%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		FOPS_MUTEX_LOCK();
		btmtk_fops_set_state(BTMTK_FOPS_STATE_CLOSED);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}
	USB_MUTEX_UNLOCK();

	btmtk_usb_stop_traffic();
	btmtk_usb_send_hci_reset_cmd();

	btmtk_usb_send_deinit_cmds();

	btmtk_usb_lock_unsleepable_lock(&(g_data->metabuffer->spin_lock));
	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;
	btmtk_usb_unlock_unsleepable_lock(&(g_data->metabuffer->spin_lock));

	FOPS_MUTEX_LOCK();
	btmtk_fops_set_state(BTMTK_FOPS_STATE_CLOSED);
	FOPS_MUTEX_UNLOCK();

	/* In case no read from stack, and close directly */
	need_reset_stack = 0;
	btmtk_usb_unify_woble_suspend(g_data);

	BTUSB_INFO("%s: OK", __func__);
	return 0;
}

static long btmtk_usb_fops_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long retval = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	switch (cmd) {
	case IOCTL_FW_ASSERT:
		/* BT trigger fw assert for debug */
		BTUSB_INFO("BT Set fw assert..., reason:%lu", arg);
		break;
	default:
		retval = -EFAULT;
		BTUSB_WARN("BT_ioctl(): unknown cmd (%d)", cmd);
		break;
	}

	return retval;
}

static unsigned int btmtk_usb_fops_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	int state = BTMTK_USB_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	if (g_data->metabuffer->read_p == g_data->metabuffer->write_p) {
		poll_wait(file, &inq, wait);

		/* empty let select sleep */
		if ((g_data->metabuffer->read_p != g_data->metabuffer->write_p) || need_reset_stack)
			mask |= POLLIN | POLLRDNORM;		/* readable */
	} else {
		mask |= POLLIN | POLLRDNORM;			/* readable */
	}

	/* do we need condition? */
	mask |= POLLOUT | POLLWRNORM;				/* writable */

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_FW_DUMP || state == BTMTK_USB_STATE_RESUME_FW_DUMP)
		mask |= POLLIN | POLLRDNORM;			/* readable */
	else if (state != BTMTK_USB_STATE_WORKING)		/* BTMTK_USB_STATE_WORKING: do nothing */
		mask = 0;
	USB_MUTEX_UNLOCK();

	return mask;
}

static ssize_t btmtk_usb_fops_readfwlog(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int copyLen = 0;
	ulong flags = 0;
	struct sk_buff *skb = NULL;

	if (g_data == NULL)
		return -ENODEV;

	/* picus read a queue, it may occur performace issue */
	FWLOG_SPIN_LOCK(flags);
	if (skb_queue_len(&g_data->fwlog_queue))
		skb = skb_dequeue(&g_data->fwlog_queue);
	FWLOG_SPIN_UNLOCK(flags);
	if (skb == NULL)
		return 0;

	if (skb->len <= count) {
		if (copy_to_user(buf, skb->data, skb->len)) {
			BTUSB_ERR("%s: copy_to_user failed!", __func__);
			/* copy_to_user failed, add skb to fwlog_fops_queue */
			skb_queue_head(&g_data->fwlog_queue, skb);
			copyLen = -EFAULT;
			goto OUT;
		}
		copyLen = skb->len;
	} else
		BTUSB_ERR("%s: socket buffer length error(count: %d, skb.len: %d)", __func__, (int)count, skb->len);

	kfree_skb(skb);
OUT:
	return copyLen;
}

static int btmtk_usb_fops_openfwlog(struct inode *inode, struct file *file)
{
	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	BTUSB_INFO("%s: OK", __func__);
	return 0;
}

static int btmtk_usb_fops_closefwlog(struct inode *inode, struct file *file)
{
	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	BTUSB_INFO("%s: OK", __func__);
	return 0;
}

static long btmtk_usb_fops_unlocked_ioctlfwlog(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	BTUSB_ERR("%s: ->", __func__);
	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	return retval;
}

static unsigned int btmtk_usb_fops_pollfwlog(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	poll_wait(file, &fw_log_inq, wait);
	if (skb_queue_len(&g_data->fwlog_queue) > 0)
		mask |= POLLIN | POLLRDNORM;			/* readable */

	return mask;
}

/*============================================================================*/
/* Interface Functions : SCO */
/*============================================================================*/
#define ___________________________________________Interface_Function_for_SCO
static ssize_t btmtk_usb_fops_sco_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	int real_num;
	int pos = 0;
	int retval = 0;
	int remain = 0;
	int multiple = 0;
	int iso_packet_size = 0;
	int i = 0;
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;
	u8 *tmp = NULL;

	if (g_data == NULL || buf == NULL || count <= 0) {
		BTUSB_ERR("%s: ERROR, %s is NULL!", __func__,
				g_data == NULL ? "g_data" : buf == NULL ? "buf" : "count");
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_DBG("%s: current is in suspend/resume (%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		return -EAGAIN;
	}
	USB_MUTEX_UNLOCK();

	BTUSB_DBG("%s start(%d)", __func__, (int)count);
	/* semaphore mechanism, the waited process will sleep */
	down(&g_data->isoc_wr_mtx);
	retval = btmtk_usb_set_isoc_interface(false);
	if (retval < 0 || g_data->isoc_urb_submitted == 0)
		goto OUT;

	if (g_data->isoc_alt_setting == ISOC_IF_ALT_MSBC)
		iso_packet_size = ISOC_HCI_PKT_SIZE_MSBC;
	else if (g_data->isoc_alt_setting == ISOC_IF_ALT_CVSD)
		iso_packet_size = ISOC_HCI_PKT_SIZE_CVSD;
	else
		iso_packet_size = ISOC_HCI_PKT_SIZE_DEFAULT;

	g_data->o_sco_buf[0] = HCI_SCODATA_PKT;
	tmp = g_data->o_sco_buf + 1;

	real_num = (BUFFER_SIZE - 1) / iso_packet_size;
	/* check remain buffer, could send more data but not a whole iso_packet_size */
	if ((BUFFER_SIZE - 1) > (real_num * iso_packet_size + HCI_SCO_HDR_SIZE))
		real_num += 1;

	/* Upper layer should take care if write size more then driver buffer */
	if (count > BUFFER_SIZE - 1 - (real_num * HCI_SCO_HDR_SIZE)) {
		BTUSB_WARN("%s: Write length more than driver buffer size(%d/%d)",
				__func__, (int)count, BUFFER_SIZE - 1 - real_num * HCI_SCO_HDR_SIZE);
		count = BUFFER_SIZE - 1 - (real_num * HCI_SCO_HDR_SIZE);
	}

	multiple = count / (iso_packet_size - HCI_SCO_HDR_SIZE);

	if (count % (iso_packet_size - HCI_SCO_HDR_SIZE))
		multiple += 1;

	remain = count;
	BTUSB_DBG("remain = %d, multiple = %d", remain, multiple);
	for (i = 0; i < multiple; i++) {
		*tmp = (u8)(sco_handle & 0x00FF);
		*(tmp + 1) = sco_handle >> 8;
		*(tmp + 2) = remain < iso_packet_size - HCI_SCO_HDR_SIZE
			? remain : iso_packet_size - HCI_SCO_HDR_SIZE;
		remain -= *(tmp + 2);
		BTUSB_DBG("remain = %d, pkt_len = %d", remain, *(tmp + 2));

		if (copy_from_user(tmp + 3, buf + pos, *(tmp + 2))) {
			retval = -EFAULT;
			BTUSB_ERR("%s: copy data from user fail", __func__);
			goto OUT;
		}
		pos += *(tmp + 2);
		tmp += (3 + *(tmp + 2));
	}
	retval = btmtk_usb_send_data(g_data->o_sco_buf,
			(count - remain) + HCI_SCO_HDR_SIZE * multiple + 1);
	if (retval > 0)
		retval = count;

 OUT:
	up(&g_data->isoc_wr_mtx);
	return retval;
}

static ssize_t btmtk_usb_fops_sco_read(struct file *file, char __user *buf,
		size_t count, loff_t *f_pos)
{
	ssize_t retval = 0;
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;
	int iso_packet_size = 0;
	struct sk_buff *skb = NULL;
	unsigned long flags = 0;

	BTUSB_DBG("%s", __func__);
	if (g_data == NULL || buf == NULL || count <= 0) {
		BTUSB_ERR("%s: ERROR, %s is NULL!", __func__,
				g_data == NULL ? "g_data" : buf == NULL ? "buf" : "count");
		return -ENODEV;
	}

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		FOPS_MUTEX_UNLOCK();
		return -ENODEV;
	}
	FOPS_MUTEX_UNLOCK();

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_ERR("%s: current is in suspend/resume (%d).", __func__, state);
		USB_MUTEX_UNLOCK();
		return -EAGAIN;
	}
	USB_MUTEX_UNLOCK();

	down(&g_data->isoc_rd_mtx);
	retval = btmtk_usb_set_isoc_interface(false);
	if (retval < 0 || g_data->isoc_urb_submitted == 0)
		goto OUT;

	/* means the buffer is empty */
	while (skb_queue_len(&g_data->isoc_in_queue) == 0) {
		/* If nonblocking mode, return directly O_NONBLOCK is specified during open() */
		if (file->f_flags & O_NONBLOCK) {
			/* BTUSB_DBG("Non-blocking btmtk_usb_fops_read()"); */
			retval = -EAGAIN;
			goto OUT;
		}
		wait_event(BT_sco_wq, skb_queue_len(&g_data->isoc_in_queue) > 0);
	}

	if (skb_queue_len(&g_data->isoc_in_queue) > 0) {
		u32 remain = 0;
		u32 shift = 0;
		u8 real_num = 0;
		u8 i = 1;

		ISOC_SPIN_LOCK(flags);
		skb = skb_dequeue(&g_data->isoc_in_queue);
		ISOC_SPIN_UNLOCK(flags);
		if (skb == NULL) {
			BTUSB_WARN("sbk is NULL");
			goto OUT;
		}

		if (g_data->isoc_alt_setting == ISOC_IF_ALT_MSBC)
			iso_packet_size = ISOC_HCI_PKT_SIZE_MSBC;
		else if (g_data->isoc_alt_setting == ISOC_IF_ALT_CVSD)
			iso_packet_size = ISOC_HCI_PKT_SIZE_CVSD;
		else
			iso_packet_size = ISOC_HCI_PKT_SIZE_DEFAULT;

		real_num = skb->len / iso_packet_size;
		if (skb->len % iso_packet_size)
			real_num += 1;
		BTUSB_DBG("real_num: %d, mod: %d", real_num, skb->len % iso_packet_size);
		if (count < skb->len - real_num * HCI_SCO_HDR_SIZE) {
			int num = count / (iso_packet_size - HCI_SCO_HDR_SIZE);
			struct sk_buff *new_skb = NULL;

			remain = skb->len - num * iso_packet_size;
			new_skb = alloc_skb(remain, GFP_KERNEL);

			memcpy(new_skb->data, skb->data + num * iso_packet_size, remain);
			new_skb->len = remain;
			ISOC_SPIN_LOCK(flags);
			skb_queue_head(&g_data->isoc_in_queue, new_skb);
			ISOC_SPIN_UNLOCK(flags);
		}
		retval = skb->len - remain;	/* Include 3 bytes header */
		shift = 0;
		while (retval > 0) {
			size_t copy = *(skb->data + ((i - 1) * HCI_SCO_HDR_SIZE) + shift + 2);

			if (copy_to_user(buf + shift, skb->data + (i * HCI_SCO_HDR_SIZE) + shift, copy))
				BTUSB_ERR("copy to user fail");

			shift += copy;
			i++;
			retval -= (HCI_SCO_HDR_SIZE + copy);
			BTUSB_DBG("copy: %d, shift: %d, retval: %d", (int)copy, (int)shift, (int)retval);
		}
		kfree_skb(skb);
		retval = shift;			/* 3 bytes header removed */
	}

OUT:
	up(&g_data->isoc_rd_mtx);
	BTUSB_DBG("Read: %d", (int)retval);
	return retval;
}

static int btmtk_usb_fops_sco_open(struct inode *inode, struct file *file)
{
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;
	unsigned long flags = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_INIT || state == BTMTK_USB_STATE_DISCONNECT) {
		USB_MUTEX_UNLOCK();
		return -EAGAIN;
	}
	USB_MUTEX_UNLOCK();

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTUSB_WARN("%s: fops not opened!", __func__);
		FOPS_MUTEX_UNLOCK();
		return 0;
	}
	FOPS_MUTEX_UNLOCK();

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING) {
		BTUSB_WARN("%s: current is in suspend/resume (%d).", __func__,
			   state);
		USB_MUTEX_UNLOCK();
		return 0;
	}
	USB_MUTEX_UNLOCK();

	atomic_set(&g_data->isoc_out_count, 0);
	ISOC_SPIN_LOCK(flags);
	skb_queue_purge(&g_data->isoc_in_queue);
	ISOC_SPIN_UNLOCK(flags);
	BTUSB_INFO("%s: OK", __func__);
	return 0;
}

static int btmtk_usb_fops_sco_close(struct inode *inode, struct file *file)
{
	unsigned long flags = 0;
	int ret = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	ret = btmtk_usb_set_isoc_interface(true);

	ISOC_SPIN_LOCK(flags);
	skb_queue_purge(&g_data->isoc_in_queue);
	ISOC_SPIN_UNLOCK(flags);
	if (ret == 0)
		BTUSB_INFO("%s: OK", __func__);
	return ret;
}

static long btmtk_usb_fops_sco_unlocked_ioctl(struct file *file, unsigned int cmd,
					  unsigned long arg)
{
	long retval = 0;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}
	return retval;
}

static unsigned int btmtk_usb_fops_sco_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	int state = BTMTK_USB_STATE_UNKNOWN;

	if (g_data == NULL) {
		BTUSB_ERR("%s: ERROR, g_data is NULL!", __func__);
		return -ENODEV;
	}

	if (skb_queue_len(&g_data->isoc_in_queue) == 0) {
		poll_wait(file, &inq_isoc, wait);

		/* empty let select sleep */
		if (skb_queue_len(&g_data->isoc_in_queue) > 0)
			mask |= POLLIN | POLLRDNORM;		/* readable */
	} else
		mask |= POLLIN | POLLRDNORM;			/* readable */

	/* do we need condition? */
	mask |= POLLOUT | POLLWRNORM;				/* writable */

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state != BTMTK_USB_STATE_WORKING)  /* BTMTK_USB_STATE_WORKING: do nothing */
		mask = 0;
	USB_MUTEX_UNLOCK();

	return mask;
}

/*============================================================================*/
/* Interface Functions : Proc */
/*============================================================================*/
#define __________________________________________Interface_Function_for_Proc
static int btmtk_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", fw_version_str);
	return 0;
}

static int btmtk_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, btmtk_proc_show, NULL);
}

static void btmtk_proc_create_new_entry(void)
{
	struct proc_dir_entry *proc_show_entry;

	BTUSB_INFO("proc initialized");
	/* /proc/stpbt/bt_fw_version */
	g_proc_dir = proc_mkdir("stpbt", 0);
	if (g_proc_dir == 0) {
		BTUSB_INFO("Unable to creat dir");
		return;
	}
	proc_show_entry = proc_create("bt_fw_version", 0644, g_proc_dir, &BT_proc_fops);
}

/*============================================================================*/
/* Interface Functions : Kernel */
/*============================================================================*/
#define ________________________________________Interface_Function_for_Kernel
static int btmtk_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_endpoint_descriptor *ep_desc;
	int i;
	int state = BTMTK_USB_STATE_UNKNOWN;
	int err = -1;

	probe_counter++;

	BTUSB_INFO("%s: begin", __func__);
	BTUSB_INFO("========================================================");
	BTUSB_INFO("btmtk_usb Mediatek Bluetooth USB driver ver %s", VERSION);
	BTUSB_INFO("========================================================");
	BTUSB_INFO("probe_counter = %d", probe_counter);

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_SUSPEND_DISCONNECT)
		btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND_PROBE);
	else if (state == BTMTK_USB_STATE_RESUME_DISCONNECT)
		btmtk_usb_set_state(BTMTK_USB_STATE_RESUME_PROBE);
	else
		btmtk_usb_set_state(BTMTK_USB_STATE_PROBE);
	USB_MUTEX_UNLOCK();

	/* interface numbers are hardcoded in the spec */
	if (intf->cur_altsetting->desc.bInterfaceNumber != 0) {
		BTUSB_ERR("[ERR] interface number != 0 (%d)", intf->cur_altsetting->desc.bInterfaceNumber);

		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
		USB_MUTEX_UNLOCK();

		BTUSB_ERR("btmtk_usb_probe end Error 1");
		return -ENODEV;
	}

	if (!g_data) {
		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
		USB_MUTEX_UNLOCK();

		BTUSB_ERR("btmtk_usb_probe end Error 2");
		return -ENOMEM;
	}

	btmtk_usb_init_memory();

	/* set the endpoint type of the interface to btmtk_usb_data */
	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		ep_desc = &intf->cur_altsetting->endpoint[i].desc;

		if (usb_endpoint_is_int_in(ep_desc)) {
			g_data->intr_ep = ep_desc;
			continue;
		}

		if (usb_endpoint_is_bulk_out(ep_desc)) {
			g_data->bulk_tx_ep = ep_desc;
			continue;
		}

		if (usb_endpoint_is_bulk_in(ep_desc)) {
			g_data->bulk_rx_ep = ep_desc;
			continue;
		}
	}

	if (!g_data->intr_ep || !g_data->bulk_tx_ep || !g_data->bulk_rx_ep) {
		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
		USB_MUTEX_UNLOCK();

		BTUSB_ERR("btmtk_usb_probe end Error 3");
		return -ENODEV;
	}

	g_data->udev = interface_to_usbdev(intf);
	g_data->intf = intf;

	spin_lock_init(&g_data->txlock);
	INIT_WORK(&g_data->waker, btmtk_usb_waker);

	g_data->meta_tx = 0;

	/* init all usb anchor */
	init_usb_anchor(&g_data->bulk_out_anchor);
	init_usb_anchor(&g_data->intr_in_anchor);
	init_usb_anchor(&g_data->bulk_in_anchor);
	init_usb_anchor(&g_data->isoc_in_anchor);
	init_usb_anchor(&g_data->isoc_out_anchor);

	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;
	memset(g_data->metabuffer->buffer, 0, META_BUFFER_SIZE);

	btmtk_usb_cap_init();

	err = btmtk_usb_load_rom_patch();
	if (err < 0) {
		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
		USB_MUTEX_UNLOCK();

		BTUSB_ERR("btmtk_usb_probe end Error 4");
		return err;
	}

	/* Interface numbers are hardcoded in the specification */
	g_data->isoc = usb_ifnum_to_if(g_data->udev, 1);

	/* bind isoc interface to usb driver */
	if (g_data->isoc) {
		err = usb_driver_claim_interface(&btmtk_usb_driver, g_data->isoc, g_data);
		if (err < 0) {
			USB_MUTEX_LOCK();
			btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
			USB_MUTEX_UNLOCK();

			BTUSB_ERR("btmtk_usb_probe end Error 7");
			return err;
		}
	}

	usb_set_intfdata(intf, g_data);

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	switch (state) {
	case BTMTK_USB_STATE_SUSPEND_FW_DUMP:
		BTUSB_INFO("%s State is BTMTK_USB_STATE_SUSPEND_FW_DUMP", __func__);
		break;

	case BTMTK_USB_STATE_RESUME_FW_DUMP:
		BTUSB_INFO("%s State is BTMTK_USB_STATE_RESUME_FW_DUMP", __func__);
		break;

	default:
		BTUSB_WARN("%s Original state is %d", __func__, state);
		btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
	}
	USB_MUTEX_UNLOCK();

	sco_handle = 0;
	g_data->isoc_urb_submitted = 0;
	memset(g_data->bdaddr, 0, BD_ADDRESS_SIZE);
	if (is_support_unify_woble(g_data)) {
		btmtk_usb_load_woble_setting(g_data->woble_setting_file_name,
			&g_data->udev->dev,
			&g_data->woble_setting_len,
			g_data);
		if (need_reset_stack)
			btmtk_usb_reset_power_on();
	}

	if (need_reset_stack) {
		BTUSB_INFO("%s: need_reset_stack %d", __func__, need_reset_stack);
		wake_up_interruptible(&inq);
	}

	btmtk_usb_woble_wake_unlock(g_data);

	BTUSB_INFO("%s: end", __func__);
	return 0;
}

static void btmtk_usb_disconnect(struct usb_interface *intf)
{
	int state = BTMTK_USB_STATE_UNKNOWN;
	int fstate = BTMTK_FOPS_STATE_UNKNOWN;

	if (!usb_get_intfdata(intf))
		return;

	BTUSB_INFO("%s: begin", __func__);

	USB_MUTEX_LOCK();
	state = btmtk_usb_get_state();
	if (state == BTMTK_USB_STATE_SUSPEND || state == BTMTK_USB_STATE_SUSPEND_DISCONNECT) {
		if (register_late_resume_func) {
			BTUSB_WARN("%s: state=%d disc happens in suspend --> should stay in suspend state later!",
					__func__, state);
			btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND_DISCONNECT);
		} else {
			BTUSB_WARN("%s: state=%d, disc happens in suspend state --> go to disconnect state!",
					__func__, state);
			btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
		}
	} else if (state == BTMTK_USB_STATE_RESUME || state == BTMTK_USB_STATE_RESUME_FW_DUMP ||
			state == BTMTK_USB_STATE_RESUME_DISCONNECT) {
		BTUSB_WARN("%s: state=%d disc happens when driver is in resume, should stay in resume state later!",
				__func__, state);
		btmtk_usb_set_state(BTMTK_USB_STATE_RESUME_DISCONNECT);
	} else
		btmtk_usb_set_state(BTMTK_USB_STATE_DISCONNECT);
	USB_MUTEX_UNLOCK();

	if (is_mt7668(g_data) || is_mt7663(g_data))
		g_data->is_mt7668_dongle_state = BTMTK_USB_7668_DONGLE_STATE_POWER_OFF;

	if (!g_data)
		return;

	FOPS_MUTEX_LOCK();
	fstate = btmtk_fops_get_state();
	if (fstate == BTMTK_FOPS_STATE_OPENED || fstate == BTMTK_FOPS_STATE_CLOSING) {
		BTUSB_WARN("%s: fstate = %d , set need_reset_stack = 1", __func__, fstate);
		need_reset_stack = 1;
	}
	FOPS_MUTEX_UNLOCK();

	if (need_reset_stack == HW_ERR_NONE)
		need_reset_stack = HW_ERR_CODE_USB_DISC;

	usb_set_intfdata(g_data->intf, NULL);

	if (g_data->isoc)
		usb_set_intfdata(g_data->isoc, NULL);

	if (intf == g_data->isoc)
		usb_driver_release_interface(&btmtk_usb_driver, g_data->intf);
	else if (g_data->isoc)
		usb_driver_release_interface(&btmtk_usb_driver, g_data->isoc);

	g_data->meta_tx = 0;
	g_data->metabuffer->read_p = 0;
	g_data->metabuffer->write_p = 0;

	cancel_work_sync(&g_data->waker);

	btmtk_usb_woble_free_setting();
	BTUSB_INFO("%s: end", __func__);
}

static int btmtk_usb_suspend(struct usb_interface *intf, pm_message_t message)
{
	int ret = -1;

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_SUSPEND);
	USB_MUTEX_UNLOCK();

	if ((g_data->suspend_count++)) {
		BTUSB_WARN("%s: Has suspended. suspend_count: %d end", __func__, g_data->suspend_count);
		return 0;
	}

	BTUSB_INFO("%s: begin", __func__);
	btmtk_usb_stop_traffic();
	usb_kill_anchored_urbs(&g_data->bulk_out_anchor);
	usb_kill_anchored_urbs(&g_data->isoc_out_anchor);

	if (!is_support_unify_woble(g_data)) {
		ret = btmtk_usb_handle_entering_WoBLE_state();
		if (ret)
			BTUSB_ERR("%s: btmtk_usb_handle_entering_WoBLE_state return fail  %d", __func__, ret);

	} else {
		btmtk_usb_unify_woble_suspend(g_data);
	}

	BTUSB_INFO("%s: end", __func__);
	return 0;
}

static int btmtk_usb_resume(struct usb_interface *intf)
{
	int ret = 0;

	g_data->suspend_count--;
	if (g_data->suspend_count) {
		BTUSB_WARN("%s: data->suspend_count %d, return 0", __func__, g_data->suspend_count);
		return 0;
	}

	BTUSB_INFO("%s: begin", __func__);

	if ((is_mt7668(g_data) || is_mt7663(g_data))
			&& g_data->is_mt7668_dongle_state == BTMTK_USB_7668_DONGLE_STATE_ERROR) {
		BTUSB_INFO("%s: In BTMTK_USB_7668_DONGLE_STATE_ERROR(Could suspend caused), do assert", __func__);
		if (need_reset_stack == HW_ERR_NONE)
			need_reset_stack = HW_ERR_CODE_WOBLE;
		btmtk_usb_send_assert_cmd();
		return -EBADFD;
	} else if ((is_mt7668(g_data) || is_mt7663(g_data))
			&& g_data->is_mt7668_dongle_state != BTMTK_USB_7668_DONGLE_STATE_WOBLE) {
		BTUSB_INFO("%s: is_mt7668_dongle_state %d return", __func__, g_data->is_mt7668_dongle_state);
		USB_MUTEX_LOCK();
		btmtk_usb_set_state(BTMTK_USB_STATE_WORKING);
		USB_MUTEX_UNLOCK();
		return 0;
	} else if (need_reset_stack) {
		BTUSB_INFO("%s: need_reset_stack", __func__);
		wake_up_interruptible(&inq);
	}

	USB_MUTEX_LOCK();
	btmtk_usb_set_state(BTMTK_USB_STATE_RESUME);
	USB_MUTEX_UNLOCK();

	ret = btmtk_usb_handle_resume();
	if (ret) {
		/* avoid rtc to to suspend again, do FW dump first */
		btmtk_usb_woble_wake_lock(g_data);
		BTUSB_ERR("%s: do assert", __func__);
		if (need_reset_stack == HW_ERR_NONE)
			need_reset_stack = HW_ERR_CODE_WOBLE;
		btmtk_usb_send_assert_cmd();
	}

	BTUSB_INFO("%s: end(%d)", __func__, ret);
	return ret;
}

#if !BT_DISABLE_RESET_RESUME
static int btmtk_usb_reset_resume(struct usb_interface *intf)
{
	BTUSB_INFO("%s: Call resume directly", __func__);
	return btmtk_usb_resume(intf);
}
#endif

static struct usb_driver btmtk_usb_driver = {
	.name = "btmtk_usb",
	.probe = btmtk_usb_probe,
	.disconnect = btmtk_usb_disconnect,
	.suspend = btmtk_usb_suspend,
	.resume = btmtk_usb_resume,
#if !BT_DISABLE_RESET_RESUME
	.reset_resume = btmtk_usb_reset_resume,
#endif
	.id_table = btmtk_usb_table,
	.supports_autosuspend = 1,
	.disable_hub_initiated_lpm = 1,
};

static int __init btmtk_usb_init(void)
{
	int retval = 0;

	BTUSB_INFO("%s: btmtk usb driver ver %s", __func__, VERSION);

	retval = btmtk_usb_BT_init();
	if (retval < 0)
		return retval;

#if ENABLE_BT_FIFO_THREAD
	btmtk_fifo_start(g_data->bt_fifo);
#endif

	retval = usb_register(&btmtk_usb_driver);
	if (retval)
		BTUSB_INFO("%s: usb registration failed!(%d)", __func__, retval);
	else
		BTUSB_INFO("%s: usb registration success.", __func__);

	return retval;
}

static void __exit btmtk_usb_exit(void)
{
	BTUSB_INFO("%s: btmtk usb driver ver %s", __func__, VERSION);

	usb_deregister(&btmtk_usb_driver);
	btmtk_usb_BT_exit();
}

module_init(btmtk_usb_init);
module_exit(btmtk_usb_exit);

/**
 * Module information
 */
MODULE_DESCRIPTION("Mediatek Bluetooth USB driver ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
