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
#ifndef __BTMTK_USB_MAIN_H__
#define __BTMTK_USB_MAIN_H__

#include <net/bluetooth/bluetooth.h>
#include "btmtk_define.h"
#if (SUPPORT_UNIFY_WOBLE & SUPPORT_ANDROID)
#include <linux/wakelock.h>
#endif

#define BD_ADDRESS_SIZE 6

enum {
	HW_ERR_NONE = 0,
	HW_ERR_CODE_CHIP_RESET = 1,
	HW_ERR_CODE_LEGACY_WOBLE = 2,
	HW_ERR_CODE_USB_DISC = 3,
	HW_ERR_CODE_CORE_DUMP = 4,
	HW_ERR_CODE_POWER_ON = 5,
	HW_ERR_CODE_POWER_OFF = 6,
	HW_ERR_CODE_WOBLE = 7,
	HW_ERR_CODE_SET_SLEEP_CMD = 8,
};

/* Please keep sync with btmtk_usb_set_state function */
enum {
	BTMTK_USB_STATE_UNKNOWN,
	BTMTK_USB_STATE_INIT,
	BTMTK_USB_STATE_DISCONNECT,
	BTMTK_USB_STATE_PROBE,
	BTMTK_USB_STATE_WORKING,
	BTMTK_USB_STATE_EARLY_SUSPEND,
	BTMTK_USB_STATE_SUSPEND,
	BTMTK_USB_STATE_RESUME,
	BTMTK_USB_STATE_LATE_RESUME,
	BTMTK_USB_STATE_FW_DUMP,
	BTMTK_USB_STATE_SUSPEND_DISCONNECT,
	BTMTK_USB_STATE_SUSPEND_PROBE,
	BTMTK_USB_STATE_SUSPEND_FW_DUMP,
	BTMTK_USB_STATE_RESUME_DISCONNECT,
	BTMTK_USB_STATE_RESUME_PROBE,
	BTMTK_USB_STATE_RESUME_FW_DUMP,
};

/* Please keep sync with btmtk_fops_set_state function */
enum {
	BTMTK_FOPS_STATE_UNKNOWN,	/* deinit in stpbt destroy */
	BTMTK_FOPS_STATE_INIT,		/* init in stpbt created */
	BTMTK_FOPS_STATE_OPENED,	/* open in fops_open */
	BTMTK_FOPS_STATE_CLOSING,	/* during closing */
	BTMTK_FOPS_STATE_CLOSED,	/* closed */
};

struct OSAL_UNSLEEPABLE_LOCK {
	spinlock_t	lock;
	unsigned long	flag;
};

struct ring_buffer_struct {
	struct OSAL_UNSLEEPABLE_LOCK	spin_lock;
	unsigned char			buffer[META_BUFFER_SIZE];	/* MTKSTP_BUFFER_SIZE:1024 */
	unsigned int			read_p;				/* indicate the current read index */
	unsigned int			write_p;			/* indicate the current write index */
};

struct woble_setting_struct {
	char	*content;	/* APCF content or radio off content */
	int	length;		/* APCF content or radio off content of length */
};


struct btmtk_usb_data {
	struct usb_device	*udev;	/* store the usb device informaiton */
	struct usb_interface	*intf;	/* current interface */
	struct usb_interface	*isoc;	/* isochronous interface */
	struct work_struct	waker;
	struct semaphore	wr_mtx;
	struct semaphore	rd_mtx;
	struct semaphore	isoc_wr_mtx;
	struct semaphore	isoc_rd_mtx;
	struct usb_anchor	intr_in_anchor;	/* intr in */
	struct usb_anchor	bulk_in_anchor;	/* bulk in */
	struct usb_anchor	bulk_out_anchor;/* bulk out */
	struct usb_anchor	isoc_in_anchor;	/* isoc in */
	struct usb_anchor	isoc_out_anchor;/* isoc out */
	int			meta_tx;
	spinlock_t		txlock;

	/* USB endpoint */
	struct usb_endpoint_descriptor	*intr_ep;
	struct usb_endpoint_descriptor	*bulk_tx_ep;
	struct usb_endpoint_descriptor	*bulk_rx_ep;
	struct usb_endpoint_descriptor  *isoc_tx_ep;
	struct usb_endpoint_descriptor  *isoc_rx_ep;

	int		suspend_count;

	/* request for different io operation */
	unsigned char	w_request;
	unsigned char	r_request;

	/* io buffer for usb control transfer */
	unsigned char	*io_buf;

	unsigned char		*rom_patch;
	unsigned char		*rom_patch_bin_file_name;
	unsigned char		*rom_patch_image;
	unsigned int		rom_patch_image_len;
	unsigned int		chip_id;
	unsigned int		rom_patch_offset;
	unsigned int		rom_patch_len;

	unsigned char		*woble_setting_file_name;
	unsigned int		woble_setting_len;

	struct woble_setting_struct		woble_setting_apcf[WOBLE_SETTING_COUNT];
	struct woble_setting_struct		woble_setting_apcf_fill_mac[WOBLE_SETTING_COUNT];
	struct woble_setting_struct		woble_setting_apcf_fill_mac_location[WOBLE_SETTING_COUNT];

	struct woble_setting_struct		woble_setting_radio_off;
	struct woble_setting_struct		woble_setting_radio_off_status_event;
	/* complete event */
	struct woble_setting_struct		woble_setting_radio_off_comp_event;

	struct woble_setting_struct		woble_setting_radio_on;
	struct woble_setting_struct		woble_setting_radio_on_status_event;
	struct woble_setting_struct		woble_setting_radio_on_comp_event;

	/* set apcf after resume(radio on) */
	struct woble_setting_struct		woble_setting_apcf_resume[WOBLE_SETTING_COUNT];
	unsigned char				bdaddr[BD_ADDRESS_SIZE];
	unsigned int				woble_need_trigger_coredump;
#if (SUPPORT_UNIFY_WOBLE & SUPPORT_ANDROID)
	struct					wake_lock woble_wlock;
#endif
	unsigned int				woble_need_set_radio_off_in_probe;

	unsigned char		*i_buf;
	unsigned char		*o_buf;
	char			*i_fwlog_buf;
	unsigned char		*o_fwlog_buf;
	unsigned char		*o_sco_buf;
	unsigned char		*o_usb_buf;

	struct ring_buffer_struct
				*metabuffer;

	unsigned char		interrupt_urb_submitted;
	unsigned char		bulk_urb_submitted;
	unsigned char		isoc_urb_submitted;
	atomic_t		isoc_out_count;

#if ENABLE_BT_FIFO_THREAD
	void			*bt_fifo;
#endif
#if SUPPORT_MT7668 || SUPPORT_MT7663
	unsigned char		is_mt7668_dongle_state;
#endif
	struct sk_buff_head	fwlog_queue;
	spinlock_t		fwlog_lock;
	struct sk_buff_head	isoc_in_queue;
	spinlock_t		isoc_lock;
	unsigned char		isoc_alt_setting;
};

/**
 * Inline functions
 */
static inline int is_support_unify_woble(struct btmtk_usb_data *data)
{
#if SUPPORT_UNIFY_WOBLE
	if (((data->chip_id & 0xffff) == 0x7668) ||
		((data->chip_id & 0xffff) == 0x7663))
		return 1;
	else
		return 0;
#else
	return 0;
#endif
}

#if SUPPORT_MT7662
static inline int is_mt7662(struct btmtk_usb_data *data)
{
	/** Return 1, if MT76x2 or MT76x2T */
	return ((data->chip_id & 0xffff0000) == 0x76620000)
		|| ((data->chip_id & 0xffff0000) == 0x76320000);
}

static inline int is_mt7662T(struct btmtk_usb_data *data)
{
	/** Return 1, if MT76x2T */
	return ((data->chip_id & 0xffffffff) == 0x76620100)
		|| ((data->chip_id & 0xffffffff) == 0x76320100);
}
#endif

#if SUPPORT_MT7668
static inline int is_mt7668(struct btmtk_usb_data *data)
{
	return ((data->chip_id & 0xffff) == 0x7668);
}
#endif

#if SUPPORT_MT7668 || SUPPORT_MT7663
enum {
	BTMTK_USB_7668_DONGLE_STATE_UNKNOWN,
	BTMTK_USB_7668_DONGLE_STATE_POWERING_ON,
	BTMTK_USB_7668_DONGLE_STATE_POWER_ON,
	BTMTK_USB_7668_DONGLE_STATE_WOBLE,
	BTMTK_USB_7668_DONGLE_STATE_POWERING_OFF,
	BTMTK_USB_7668_DONGLE_STATE_POWER_OFF,
	BTMTK_USB_7668_DONGLE_STATE_ERROR
};
#endif

#if SUPPORT_MT7663
static inline int is_mt7663(struct btmtk_usb_data *data)
{
	return ((data->chip_id & 0xffff) == 0x7663);
}
#endif


/**
 * Extern functions
 */
int btmtk_usb_send_data(const unsigned char *buffer, const unsigned int length);
int btmtk_usb_meta_send_data(const unsigned char *buffer,
		const unsigned int length);
void btmtk_usb_toggle_rst_pin(void);

#endif /* __BTMTK_USB_MAIN_H__ */
