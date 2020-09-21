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

#ifndef LIBDVBCA_H
#define LIBDVBCA_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * The types of CA interface we support.
 */
#define DVBCA_INTERFACE_LINK 0
#define DVBCA_INTERFACE_HLCI 1

/**
 * States a CAM in a slot can be in.
 */
#define DVBCA_CAMSTATE_MISSING 0
#define DVBCA_CAMSTATE_INITIALISING 1
#define DVBCA_CAMSTATE_READY 2


/**
 * Open a CA device. Multiple CAMs can be accessed through a CA device.
 *
 * @param adapter Index of the DVB adapter.
 * @param cadevice Index of the CA device on that adapter (usually 0).
 * @return A unix file descriptor on success, or -1 on failure.
 */
extern int dvbca_open(int adapter, int cadevice);

/**
 * Reset a CAM.
 *
 * @param fd File handle opened with dvbca_open.
 * @param slot Slot where the requested CAM is in.
 * @return 0 on success, -1 on failure.
 */
extern int dvbca_reset(int fd, uint8_t slot);

/**
 * Get the interface type of a CAM.
 *
 * @param fd File handle opened with dvbca_open.
 * @param slot Slot where the requested CAM is in.
 * @return One of the DVBCA_INTERFACE_* values, or -1 on failure.
 */
extern int dvbca_get_interface_type(int fd, uint8_t slot);

/**
 * Get the state of a CAM.
 *
 * @param fd File handle opened with dvbca_open.
 * @param slot Slot where the requested CAM is in.
 * @return One of the DVBCA_CAMSTATE_* values, or -1 on failure.
 */
extern int dvbca_get_cam_state(int fd, uint8_t slot);

/**
 * Write a message to a CAM using a link-layer interface.
 *
 * @param fd File handle opened with dvbca_open.
 * @param slot Slot where the requested CAM is in.
 * @param connection_id Connection ID of the message.
 * @param data Data to write.
 * @param data_length Number of bytes to write.
 * @return 0 on success, or -1 on failure.
 */
extern int dvbca_link_write(int fd, uint8_t slot, uint8_t connection_id,
			    uint8_t *data, uint16_t data_length);

/**
 * Read a message from a CAM using a link-layer interface.
 *
 * @param fd File handle opened with dvbca_open.
 * @param slot Slot where the responding CAM is in.
 * @param connection_id Destination for the connection ID the message came from.
 * @param data Data that was read.
 * @param data_length Max number of bytes to read.
 * @return Number of bytes read on success, or -1 on failure.
 */
extern int dvbca_link_read(int fd, uint8_t *slot, uint8_t *connection_id,
			   uint8_t *data, uint16_t data_length);

// FIXME how do we determine which CAM slot of a CA is meant?
/**
 * Write a message to a CAM using an HLCI interface.
 *
 * @param fd File handle opened with dvbca_open.
 * @param data Data to write.
 * @param data_length Number of bytes to write.
 * @return 0 on success, or -1 on failure.
 */
extern int dvbca_hlci_write(int fd, uint8_t *data, uint16_t data_length);

// FIXME how do we determine which CAM slot of a CA is meant?
/**
 * Read a message from a CAM using an HLCI interface.
 *
 * @param fd File handle opened with dvbca_open.
 * @param app_tag Application layer tag giving the message type to read.
 * @param data Data that was read.
 * @param data_length Max number of bytes to read.
 * @return Number of bytes read on success, or -1 on failure.
 */
extern int dvbca_hlci_read(int fd, uint32_t app_tag, uint8_t *data,
			   uint16_t data_length);

#ifdef __cplusplus
}
#endif

#endif // LIBDVBCA_H
