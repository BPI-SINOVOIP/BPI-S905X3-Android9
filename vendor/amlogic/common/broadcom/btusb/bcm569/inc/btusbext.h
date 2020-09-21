/*
 *
 * btusbext.h
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

#ifndef BTUSBEXT_H
#define BTUSBEXT_H

#include <linux/time.h>

// BTUSB Statistics structure
typedef struct
{
    // All URB submit counter
    unsigned long urb_submit_ok;
    unsigned long urb_submit_err;

    // ACL RX counters
    unsigned long acl_rx_submit_ok;
    unsigned long acl_rx_submit_err;
    unsigned long acl_rx_completed;
    unsigned long acl_rx_resubmit;
    unsigned long acl_rx_bytes;

    // DIAG RX counters
    unsigned long diag_rx_submit_ok;
    unsigned long diag_rx_submit_err;
    unsigned long diag_rx_completed;
    unsigned long diag_rx_resubmit;
    unsigned long diag_rx_bytes;

    // EVENT counters
    unsigned long event_submit_ok;
    unsigned long event_submit_err;
    unsigned long event_completed;
    unsigned long event_resubmit;
    unsigned long event_bytes;

    // Number of Write IRPs submitted
    unsigned long writes_submitted;

    // Number of Write IRPs submitted in error
    unsigned long writes_submitted_error;

    // Number of Write IRPs completed
    unsigned long writes_completed;

    // Number of Write IRPs completed in error
    unsigned long writes_completed_error;

    // Number of Command IRPs submitted
    unsigned long commands_submitted;

    // Number of Command IRPs submitted in error
    unsigned long commands_submitted_error;

    // Number of Command IRPs completed
    unsigned long commands_completed;

    // Number of Command IRPs completed in error
    unsigned long commands_completed_error;

    // Number of voice reqs submitted to the USB software stack
    unsigned long voicerx_submitted;

    // Number of voice rx submitted in error
    unsigned long voicerx_submitted_error;

    // Number of voice req completions from the USB software stack
    unsigned long voicerx_completed;

    // Number of Voice req completions in error
    unsigned long voicerx_completed_error;

    // Number of bad voice RX packets received
    unsigned long voicerx_bad_packets;

    // Bytes received altogether
    unsigned long voicerx_raw_bytes;

    // Bytes skipped
    unsigned long voicerx_skipped_bytes;

    // SCO headers split across packets
    unsigned long voicerx_split_hdr;

    // Voice frames discarded due to no headers in data
    unsigned long voicerx_disc_nohdr;

    // Number of Voice Tx reqs submitted
    unsigned long voicetx_submitted;

    // Number of voice tx submitted in error
    unsigned long voicetx_submitted_error;

    // Number of Voice Tx reqs completed
    unsigned long voicetx_completed;

    // Number of Voice Tx reqs completed in error
    unsigned long voicetx_completed_error;

    // Number of Voice Tx not submitted due to no room on the tx queue
    unsigned long voicetx_disc_nobuf;

    // Number of Voice tx not submitted due to too long data
    unsigned long voicetx_disc_toolong;

    /* number of Voice packet pending */
    unsigned long voice_tx_cnt;

    /* max number of Voice packet pending */
    unsigned long voice_max_tx_cnt;

    /* max delta time between 2 consecutive tx done routine in us */
    unsigned long voice_max_tx_done_delta_time;
    unsigned long voice_min_tx_done_delta_time;
    struct timeval voice_tx_done_delta_time;
    struct timeval voice_last_tx_done_ts;

    /* max delta time between 2 consecutive tx done routine in us */
    unsigned long voice_max_rx_rdy_delta_time;
    unsigned long voice_min_rx_rdy_delta_time;
    struct timeval voice_rx_rdy_delta_time;
    struct timeval voice_last_rx_rdy_ts;

    /* max delta time between 2 consecutive tx done routine in us */
    unsigned long voice_max_rx_feeding_interval;
    unsigned long voice_min_rx_feeding_interval;
    struct timeval voice_rx_feeding_interval;
    struct timeval voice_last_rx_feeding_ts;
} tBTUSB_STATS;

//
// IOCTL definitions (shared among all user mode applications, do not modify)
//
#define IOCTL_BTWUSB_GET_STATS            0x1001
#define IOCTL_BTWUSB_CLEAR_STATS          0x1002
#define IOCTL_BTWUSB_PUT                  0x1003
#define IOCTL_BTWUSB_PUT_CMD              0x1004
#define IOCTL_BTWUSB_GET                  0x1005
#define IOCTL_BTWUSB_GET_EVENT            0x1006
#define IOCTL_BTWUSB_PUT_VOICE            0x1007
#define IOCTL_BTWUSB_GET_VOICE            0x1008
#define IOCTL_BTWUSB_ADD_VOICE_CHANNEL    0x1009
#define IOCTL_BTWUSB_REMOVE_VOICE_CHANNEL 0x100a
#define IOCTL_BTWUSB_DEV_RESET            0x100b

#endif // BTUSBEXT_H
