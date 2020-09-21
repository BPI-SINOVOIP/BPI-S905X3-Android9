/*
 *
 * btusb_lite_hci.h
 *
 *
 *
 * Copyright (C) 2011-2013 Broadcom Corporation.
 *
 *
 *
 * This software is licensed under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation (the "GPL"), and may
 * be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GPL for more details.
 *
 *
 * A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php
 * or by writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA
 *
 *
 */

#ifndef BTUSB_LITE_HCI_H
#define BTUSB_LITE_HCI_H

/* Definitions for HCI ACL packets  */

/* HCI ACL Packet: HCI_Type, ConHdl, Length */
#define BTUSB_LITE_HCI_ACL_HDR_SIZE     (sizeof(UINT8) + sizeof(UINT16) + sizeof(UINT16))

/*******************************************************************************
 **
 ** Function         btusb_lite_hci_acl_send
 **
 ** Description      Send an ACL packet to HCI
 **
 ** Returns          Status
 **
 *******************************************************************************/
int btusb_lite_hci_acl_send(struct btusb_cb *p_dev, BT_HDR *p_msg, UINT16 con_hdl);

/*******************************************************************************
 **
 ** Function        btusb_lite_hci_cmd_filter
 **
 ** Description     Check if the Sent HCI Command need to be handled/caught (not
 **                 sent to BT controller).
 **
 ** Returns         status: <> 0 if the command must be send to BT controller
 **                         0 if the command is handled
 **
 *******************************************************************************/
int btusb_lite_hci_cmd_filter(struct btusb_cb *p_dev, BT_HDR *p_msg);

/*******************************************************************************
 **
 ** Function        btusb_lite_hci_event_filter
 **
 ** Description     Handle/Filter HCI Events received from BT controller
 **
 ** Returns         int (1 if HCI is over IPC otherwise 0)
 **
 *******************************************************************************/
int btusb_lite_hci_event_filter(struct btusb_cb *p_dev, UINT8 *p_data, int length);

#endif /* BTUSB_LITE_HCI_H */

