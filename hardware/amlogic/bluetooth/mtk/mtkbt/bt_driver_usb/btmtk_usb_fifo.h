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
#ifndef __BTMTK_USB_FIFO_H__
#define __BTMTK_USB_FIFO_H__
#include "btmtk_config.h"

#if ENABLE_BT_FIFO_THREAD
#include <linux/kfifo.h>	/* Used for kfifo */

#define FW_DUMP_END_EVENT "coredump end"

enum {
	FIFO_SYSLOG = 0,
	FIFO_COREDUMP,
	FIFO_END
};

enum {
	TSK_INIT = 0,
	TSK_START,
	TSK_RUNNING,
	TSK_SHOULD_STOP,
	TSK_EXIT,
};

struct btfio_stats {
	unsigned int rx;
	unsigned int tx;
};

struct btmtk_fifo_t {
	unsigned int		type;
	unsigned int		size;
	const char		*folder_name;
	struct __kfifo		kfifo;
	struct file		*filp;
	unsigned char		tried_all_path;	/* For all of customer & default path setting */
	struct btfio_stats	stat;
};

struct btmtk_fifo_data_t {
	struct task_struct	*fifo_tsk;
	struct btmtk_fifo_t	*fifo_l;
	wait_queue_head_t	rx_wait_q;
	unsigned long		tsk_flag;
};

/**
 * Extern functions
 */
void *btmtk_fifo_init(void);
int btmtk_fifo_start(void *fio_d);
u32 btmtk_fifo_in(unsigned int type, void *fifo_d, const void *buf,
		unsigned int length);

#endif /* ENABLE_BT_FIFO_THREAD */
#endif /* __BTMTK_USB_FIFO_H__ */
