#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*
 * libdvbca - interface onto raw CA devices
 *
 * Copyright (C) 2006 Andrew de Quincey (adq_dvb@lidskialf.net)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <linux/dvb/ca.h>
#include "dvbca.h"


int dvbca_open(int adapter, int cadevice)
{
	char filename[PATH_MAX+1];
	int fd;

	snprintf(filename, sizeof(filename), "/dev/dvb/adapter%i/ca%i", adapter, cadevice);
	if ((fd = open(filename, O_RDWR)) < 0) {
		// if that failed, try a flat /dev structure
		snprintf(filename, sizeof(filename), "/dev/dvb%i.ca%i", adapter, cadevice);
		fd = open(filename, O_RDWR);
	}

	return fd;
}

int dvbca_reset(int fd, uint8_t slot)
{
	return ioctl(fd, CA_RESET, (1 << slot));
}

int dvbca_get_interface_type(int fd, uint8_t slot)
{
	ca_slot_info_t info;

	info.num = slot;
	if (ioctl(fd, CA_GET_SLOT_INFO, &info))
		return -1;

	if (info.type & CA_CI_LINK)
		return DVBCA_INTERFACE_LINK;
	if (info.type & CA_CI)
		return DVBCA_INTERFACE_HLCI;

	return -1;
}

int dvbca_get_cam_state(int fd, uint8_t slot)
{
	ca_slot_info_t info;

	info.num = slot;
	if (ioctl(fd, CA_GET_SLOT_INFO, &info))
		return -1;

	if (info.flags == 0)
		return DVBCA_CAMSTATE_MISSING;
	if (info.flags & CA_CI_MODULE_READY)
		return DVBCA_CAMSTATE_READY;
	if (info.flags & CA_CI_MODULE_PRESENT)
		return DVBCA_CAMSTATE_INITIALISING;

	return -1;
}

int dvbca_link_write(int fd, uint8_t slot, uint8_t connection_id,
		     uint8_t *data, uint16_t data_length)
{
	uint8_t *buf = malloc(data_length + 2);
	if (buf == NULL)
		return -1;

	buf[0] = slot;
	buf[1] = connection_id;
	memcpy(buf+2, data, data_length);

	int result = write(fd, buf, data_length+2);
	free(buf);
	return result;
}

int dvbca_link_read(int fd, uint8_t *slot, uint8_t *connection_id,
		     uint8_t *data, uint16_t data_length)
{
	int size;

	uint8_t *buf = malloc(data_length + 2);
	if (buf == NULL)
		return -1;

	if ((size = read(fd, buf, data_length+2)) < 2)
		return -1;

	*slot = buf[0];
	*connection_id = buf[1];
	memcpy(data, buf+2, size-2);
	free(buf);

	return size - 2;
}

int dvbca_hlci_write(int fd, uint8_t *data, uint16_t data_length)
{
	struct ca_msg msg;

	if (data_length > 256) {
		return -1;
	}
	memset(&msg, 0, sizeof(msg));
	msg.length = data_length;

	memcpy(msg.msg, data, data_length);

	return ioctl(fd, CA_SEND_MSG, &msg);
}

int dvbca_hlci_read(int fd, uint32_t app_tag, uint8_t *data,
		    uint16_t data_length)
{
	struct ca_msg msg;

	if (data_length > 256) {
		data_length = 256;
	}
	memset(&msg, 0, sizeof(msg));
	msg.length = data_length;
	msg.msg[0] = app_tag >> 16;
	msg.msg[1] = app_tag >> 8;
	msg.msg[2] = app_tag;

	int status = ioctl(fd, CA_GET_MSG, &msg);
	if (status < 0) return status;

	if (msg.length > data_length) msg.length = data_length;
	memcpy(data, msg.msg, msg.length);
	return msg.length;
}
