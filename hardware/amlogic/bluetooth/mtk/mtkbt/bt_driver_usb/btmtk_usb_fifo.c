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
#include "btmtk_config.h"

#if ENABLE_BT_FIFO_THREAD
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <asm/uaccess.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci.h>
#include <linux/signal.h>
#include <linux/time.h>
#include <linux/string.h>

#include "btmtk_usb_fifo.h"
#include "btmtk_usb_main.h"

/**
 * Definition
 */
#define FW_DUMP_SIZE (1024 * 16)
#define SYS_LOG_SIZE (1024 * 16)

#define KFIO_D(pData, t)	(((struct btmtk_fifo_data_t *)(pData))->fifo_l[t].kfifo.data)
#define pKFIO(pData, t)		(&((struct btmtk_fifo_data_t *)(pData))->fifo_l[t].kfifo)
#define pFIO(pData, t)		(&((struct btmtk_fifo_data_t *)(pData))->fifo_l[t])

static struct btmtk_fifo_data_t g_fifo_data;
struct timer_list coredump_timer;

static struct btmtk_fifo_t btmtk_fifo_list[] = {
	{ FIFO_SYSLOG, SYS_LOG_SIZE, SYS_LOG_FILE_NAME, {0}, NULL, 0, {0} },
	{ FIFO_COREDUMP, FW_DUMP_SIZE, FW_DUMP_FILE_NAME, {0}, NULL, 0, {0} },
	{ FIFO_END, 0, NULL, {0}, NULL, 0, {0} },
};

static void btmtk_coredump_timo_func(void *data)
{
	struct btmtk_fifo_t *cd = NULL;

	if (data) {
		cd = (struct btmtk_fifo_t *)pFIO(data, FIFO_COREDUMP);
		cd->tried_all_path = 0;
		if (cd->filp) {
			filp_close(cd->filp, NULL);
			cd->filp = NULL;
		}
	}
	/* trigger HW reset */
	BTUSB_WARN("%s: coredump may fail, hardware reset directly", __func__);
	btmtk_usb_toggle_rst_pin();
}

static void btmtk_add_timer(struct timer_list *timer, void *fun, u16 sec, void *data)
{
	if (timer == NULL || sec == 0) {
		BTUSB_ERR("%s: Incorrect timer(%p, %d)", __func__, timer, sec);
		return;
	}
	BTUSB_INFO("%s", __func__);

	if (!timer_pending(timer)) {
		if (fun == NULL) {
			BTUSB_ERR("%s: Incorrect func for timer", __func__);
			return;
		}
		BTUSB_DBG("Add new timer");
		timer->function = fun;
		timer->data = data ? (unsigned long)data : (unsigned long)NULL;
		timer->expires = jiffies + HZ * sec;
		add_timer(timer);
	} else {
		BTUSB_DBG("Modify the timer");
		mod_timer(timer, jiffies + HZ * sec);
	}
}

static void btmtk_del_timer(struct timer_list *timer)
{
	if (timer == NULL) {
		BTUSB_ERR("%s: Incorrect timer", __func__);
		return;
	}
	BTUSB_INFO("%s", __func__);

	del_timer_sync(timer);
}

static int fifo_alloc(struct btmtk_fifo_t *bt_fifo)
{
	int ret = 0;
	struct __kfifo *kfio = &bt_fifo->kfifo;

	ret = __kfifo_alloc(kfio, bt_fifo->size, 1, GFP_KERNEL);
	if (ret == 0)
		bt_fifo->size = kfio->mask + 1;

	return ret;
}

static int fifo_init(struct btmtk_fifo_t *bt_fifo)
{
	int ret = 0;
	struct __kfifo *kfio = &bt_fifo->kfifo;

	BTUSB_INFO("%s: %d", __func__, bt_fifo->type);

	ret = __kfifo_init(kfio, kfio->data, bt_fifo->size, kfio->esize);

	if (bt_fifo->filp) {
		BTUSB_INFO("%s: call filp_close", __func__);
		vfs_fsync(bt_fifo->filp, 0);
		filp_close(bt_fifo->filp, NULL);
		bt_fifo->filp = NULL;
		BTUSB_WARN("%s: FW dump file closed, rx=0x%X, tx =0x%x",
				__func__, bt_fifo->stat.rx, bt_fifo->stat.tx);
	}

	bt_fifo->stat.rx = 0;
	bt_fifo->stat.tx = 0;
	return ret;
}

static void fifo_out_info(struct btmtk_fifo_data_t *data_p)
{
	struct __kfifo *fifo = NULL;

	if (data_p == NULL) {
		BTUSB_ERR("%s: data_p == NULL return", __func__);
		return;
	}

	fifo = pKFIO(data_p, FIFO_SYSLOG);

	if (fifo->data != NULL) {
		if (fifo->in > fifo->out + fifo->mask)
			BTUSB_WARN("sys_log buffer full");

		if (fifo->in == fifo->out + fifo->mask)
			BTUSB_WARN("sys_log buffer empty");

		BTUSB_DBG("[syslog][fifo in - fifo out = 0x%x]", fifo->in - fifo->out);
	}

	fifo = pKFIO(data_p, FIFO_COREDUMP);

	if (fifo->data != NULL) {
		if (fifo->in > fifo->out + fifo->mask)
			BTUSB_WARN("coredump buffer full");

		if (fifo->in == fifo->out + fifo->mask)
			BTUSB_WARN("coredump buffer empty");
	}
}

static u32 fifo_out_accessible(struct btmtk_fifo_data_t *data_p)
{
	int i;
	struct btmtk_fifo_t *fio_p = NULL;

	if (data_p == NULL)
		return 0;

	if (test_bit(TSK_SHOULD_STOP, &data_p->tsk_flag)) {
		BTUSB_WARN("%s: task should stop", __func__);
		return 1;
	}

	for (i = 0; i < FIFO_END; i++) {

		fio_p = &data_p->fifo_l[i];

		if (fio_p->type != i)
			continue;

		if (fio_p->kfifo.data != NULL) {
			if (fio_p->kfifo.in != fio_p->kfifo.out)
				return 1;
		}
	}

	return 0;
}

static struct file *fifo_filp_open(const char *file, int flags, umode_t mode, unsigned int type)
{
	struct file *f = NULL;
	/* Retry to open following path */
	static const char * const sys[] = {
		"/sdcard/"SYSLOG_FNAME, "/data/misc/bluedroid/"SYSLOG_FNAME, "/tmp/"SYSLOG_FNAME};
	static const char * const fw[] = {
		"/sdcard/"FWDUMP_FNAME, "/data/misc/bluedroid/"FWDUMP_FNAME, "/tmp/"FWDUMP_FNAME};
	u8 i = 0;

	f = filp_open(file, flags, mode);
	if (IS_ERR(f)) {
		const char *p = NULL;
		u8 count = ARRAY_SIZE(sys);

		f = NULL;
		BTUSB_ERR("%s: open file error: %s, try to open others default path",
				__func__, file);

		if (type != FIFO_SYSLOG && type != FIFO_COREDUMP) {
			BTUSB_ERR("%s: Incorrect type: %d", __func__, type);
			return NULL;
		}

		for (i = 0; i < count; ++i) {
			if (type == FIFO_SYSLOG)
				p = sys[i];
			else
				p = fw[i];

			if (memcmp(file, p, MIN(strlen(file), strlen(p))) != 0) {
				BTUSB_INFO("%s: Try to open %s", __func__, p);
				f = filp_open(p, flags, mode);
				if (IS_ERR(f)) {
					f = NULL;
					continue;
				}
			} else {
				continue;
			}
			BTUSB_INFO("%s: %s opened", __func__, p);
			break;
		}
	}
	return f;
}

static u32 fifo_out_to_file(struct btmtk_fifo_t *fio, u32 len, u32 off, u8 *end)
{
	struct __kfifo *fifo = &fio->kfifo;
	unsigned int size = fifo->mask + 1;
	unsigned int esize = fifo->esize;
	unsigned int l;
	mm_segment_t old_fs;

	off &= fifo->mask;

	if (esize != 1) {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);

	if (l > 0 && fifo->data != NULL) {
		old_fs = get_fs();
		set_fs(KERNEL_DS);

		if (fio->filp == NULL && fio->tried_all_path == 0) {
			BTUSB_WARN("%s: FW Dump started, try to open file %s", __func__,
				fio->folder_name);

			fio->filp = fifo_filp_open(fio->folder_name, O_RDWR | O_CREAT, 0644, fio->type);
			if (fio->filp == NULL) {
				set_fs(old_fs);
				fio->tried_all_path = 1;
				BTUSB_ERR("%s: Dump is failed to save, please check SELinux mode(# setenforce 0)",
						__func__);
				return 0;
			}
		}

		if (fio->filp != NULL) {
			/**
			 * Actually there is one dump after "coredump end", that is "TotalTimeForDump=..."
			 * char cde[] = {';', ' ', 'c', 'o', 'r', 'e', 'd', 'u', 'm', 'p', ' ', 'e', 'n', 'd'};
			 */
			char cde[] = {';', 'T', 'o', 't', 'a', 'l', 'T', 'i', 'm', 'e'};

			vfs_write(fio->filp, fifo->data + off, l, &fio->filp->f_pos);
			vfs_write(fio->filp, fifo->data, len - l, &fio->filp->f_pos);
			fio->stat.tx += len;
			BTUSB_INFO("%s: %p", __func__, fio);
			if (memcmp(fifo->data + off, cde, sizeof(cde)) == 0) {
				*end = 1;
				fio->tried_all_path = 0;
				BTUSB_INFO("%s: FW Dump finished(success)", __func__);
				filp_close(fio->filp, NULL);
				fio->filp = NULL;
			}
		}
		set_fs(old_fs);
	}

	/*
	 * make sure that the data is copied before
	 * incrementing the fifo->out index counter
	 */
	smp_wmb();
	return len;
}

static u32 fifo_out_peek_fs(struct btmtk_fifo_t *fio, u32 len, u8 *end)
{
	unsigned int l;
	struct __kfifo *fifo = &fio->kfifo;

	l = fifo->in - fifo->out;
	if (len > l)
		len = l;

	fifo_out_to_file(fio, len, fifo->out, end);

	return len;
}

static u32 fifo_out_fs(void *fifo_d)
{
	u8 end = 0;
	u32 len = 0;
	struct btmtk_fifo_t *syslog_p = NULL;
	struct btmtk_fifo_t *coredump_p = NULL;

	if (fifo_d == NULL) {
		BTUSB_ERR("[%s: fifo data is null]", __func__);
		return len;
	}

	if (KFIO_D(fifo_d, FIFO_SYSLOG) != NULL) {
		syslog_p = pFIO(fifo_d, FIFO_SYSLOG);
		len = fifo_out_peek_fs(syslog_p, SYS_LOG_SIZE, &end);
		syslog_p->kfifo.out += len;
	}

	if (KFIO_D(fifo_d, FIFO_COREDUMP) != NULL) {
		coredump_p = pFIO(fifo_d, FIFO_COREDUMP);
		len = fifo_out_peek_fs(coredump_p, FW_DUMP_SIZE, &end);
		coredump_p->kfifo.out += len;

		if (end) {
			BTUSB_INFO("%s: call reset pin", __func__);
			btmtk_del_timer(&coredump_timer);
			btmtk_usb_toggle_rst_pin();
		}
	}

	return len;
}

static int btmtk_fifo_thread(void *ptr)
{
	struct btmtk_fifo_data_t *data_p = (struct btmtk_fifo_data_t *)ptr;

	if (data_p == NULL) {
		BTUSB_ERR("%s: [FIFO Data is null]", __func__);
		return -1;
	}

	set_freezable();
	while (!kthread_should_stop() || test_bit(TSK_START, &data_p->tsk_flag)) {
		wait_event_freezable(data_p->rx_wait_q,
				fifo_out_accessible(data_p) || kthread_should_stop());
		if (test_bit(TSK_SHOULD_STOP, &data_p->tsk_flag)) {
			BTUSB_INFO("%s loop break", __func__);
			break;
		}

		fifo_out_info(data_p);
		fifo_out_fs(data_p);
		fifo_out_info(data_p);
	}

	set_bit(TSK_EXIT, &data_p->tsk_flag);
	BTUSB_INFO("%s: end: down != 0", __func__);

	return 0;
}

void *btmtk_fifo_init(void)
{
	int i;
	struct btmtk_fifo_data_t *data_p = &g_fifo_data;
	struct btmtk_fifo_t *fio_p = NULL;

	data_p->fifo_l = btmtk_fifo_list;

	while (test_bit(TSK_INIT, &data_p->tsk_flag)) {
		clear_bit(TSK_INIT, &data_p->tsk_flag);
		if (test_bit(TSK_EXIT, &data_p->tsk_flag))
			break;

		set_bit(TSK_SHOULD_STOP, &data_p->tsk_flag);
		wake_up_process(data_p->fifo_tsk);
		msleep(100);
	}

	data_p->tsk_flag = 0;

	for (i = 0; i < FIFO_END; i++) {
		fio_p = &(data_p->fifo_l[i]);

		if (fio_p->kfifo.data == NULL)
			fifo_alloc(fio_p);

		fifo_init(fio_p);
	}

	init_waitqueue_head(&data_p->rx_wait_q);
	set_bit(TSK_INIT, &data_p->tsk_flag);

	return (void *)(data_p);
}

u32 btmtk_fifo_in(unsigned int type, void *fifo_d, const void *buf,
			 unsigned int length)
{
	u32 len = 0;
	u8 hci_pkt = HCI_EVENT_PKT;
	struct btmtk_fifo_data_t *data_p = (struct btmtk_fifo_data_t *)fifo_d;
	struct btmtk_fifo_t *fio_p = NULL;
	struct __kfifo *fifo = NULL;

	if (fifo_d == NULL) {
		BTUSB_ERR("%s: [fifo data is null]", __func__);
		return len;
	}

	if (!test_bit(TSK_START, &data_p->tsk_flag)) {
		BTUSB_ERR("%s: Fail task not start", __func__);
		return 0;
	}

	fio_p = pFIO(fifo_d, type);
	fifo = pKFIO(fifo_d, type);

	if (fifo->data != NULL) {
		if (type == FIFO_SYSLOG) {
			len = __kfifo_in(pKFIO(fifo_d, type), (const void *)&hci_pkt,
					sizeof(hci_pkt));

			if (len != sizeof(hci_pkt))
				return len;
		}

		len = __kfifo_in(pKFIO(fifo_d, type), buf, length);
		fio_p->stat.rx += len;
	}

	if (len != 0) {
		wake_up_interruptible(&data_p->rx_wait_q);
		btmtk_add_timer(&coredump_timer, btmtk_coredump_timo_func, 1, data_p);
	}

	return len;
}

int btmtk_fifo_start(void *fio_d)
{
	struct btmtk_fifo_data_t *data_p = (struct btmtk_fifo_data_t *)fio_d;
	int err;

	if (data_p == NULL) {
		BTUSB_ERR("%s: [fail][fifo data is null]", __func__);
		return -1;
	}

	if (!test_bit(TSK_INIT, &data_p->tsk_flag)) {
		BTUSB_ERR("%s: [fail task not init ]", __func__);
		return -1;
	}

	if (!KFIO_D(data_p, FIFO_SYSLOG) && !KFIO_D(data_p, FIFO_COREDUMP)) {
		BTUSB_ERR("%s: There are no syslog or coredump data", __func__);
		return -1;
	}

	data_p->fifo_tsk = kthread_create(btmtk_fifo_thread, fio_d,
			"btmtk_fifo_thread");
	if (IS_ERR(data_p->fifo_tsk)) {
		BTUSB_ERR("%s: create FIFO thread failed!", __func__);
		err = PTR_ERR(data_p->fifo_tsk);
		data_p->fifo_tsk = NULL;
		return -1;
	}
	init_timer(&coredump_timer);

	set_bit(TSK_START, &data_p->tsk_flag);
	BTUSB_INFO("%s: set TSK_START", __func__);
	wake_up_process(data_p->fifo_tsk);
	BTUSB_INFO("%s: [ok]", __func__);

	return 0;
}
#endif /* ENABLE_BT_FIFO_THREAD */
