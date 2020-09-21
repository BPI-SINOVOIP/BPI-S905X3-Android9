/*
 *
 * btusb_isoc.c
 *
 *
 *
 * Copyright (C) 2013 Broadcom Corporation.
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

// for kmalloc
#include <linux/slab.h>
#include "btusb.h"
#include "hcidefs.h"

/*******************************************************************************
 **
 ** Function         btusb_isoc_reset_msg
 **
 ** Description      Reset the currently receiving voice message
 **
 ** Parameters       p_dev: device instance control block
 **
 ** Returns          void
 **
 *******************************************************************************/
static void btusb_isoc_reset_msg(tBTUSB_CB *p_dev)
{
    p_dev->pending_bytes = 0;
    if (p_dev->pp_pending_msg && *p_dev->pp_pending_msg)
    {
        GKI_freebuf(*p_dev->pp_pending_msg);
    }
    p_dev->pp_pending_msg = NULL;
    p_dev->pending_hdr_size = 0;
}

/*******************************************************************************
 **
 ** Function         btusb_isoc_check_hdr
 **
 ** Description      Check the packet header
 **
 ** Parameters       p_dev: device instance control block
 **
 ** Returns          void
 **
 *******************************************************************************/
static bool btusb_isoc_check_hdr(tBTUSB_CB *p_dev)
{
    unsigned char *p_data = p_dev->pending_hdr;
    int idx;
    unsigned short sco_handle;
    unsigned char size;
    BT_HDR *p_hdr;
    tBTUSB_VOICE_CHANNEL *p_chan;

    STREAM_TO_UINT16(sco_handle, p_data);
    sco_handle &= 0x0fff;
    STREAM_TO_UINT8(size, p_data);

    for (idx = 0; idx < ARRAY_SIZE(p_dev->voice_channels); idx++)
    {
        p_chan = &p_dev->voice_channels[idx];
        if ((p_chan->used) &&
            (sco_handle == p_chan->info.sco_handle) &&
            (size <= (2 * p_chan->info.burst)))
        {
            // check if there is already a message being consolidated
            if (unlikely(p_chan->p_msg == NULL))
            {
                p_hdr = (BT_HDR *) GKI_getpoolbuf(HCI_SCO_POOL_ID);
                if (unlikely(p_hdr == NULL))
                {
                    return false;
                }
                p_hdr->event = BT_EVT_TO_BTU_HCI_SCO;
                p_hdr->len = 1 + BTUSB_VOICE_HEADER_SIZE; // sizeof(data_type) + SCO header
                p_hdr->offset = 0;
                p_hdr->layer_specific = 0;

                p_data = (void *) (p_hdr + 1);

                // add data type
                UINT8_TO_STREAM(p_data, HCIT_TYPE_SCO_DATA);
                UINT16_TO_STREAM(p_data, sco_handle);

                p_chan->p_msg = p_hdr;
            }
            p_dev->pending_bytes = size;
            p_dev->pp_pending_msg = &p_chan->p_msg;

            return true;
        }
    }
    return false;
}

/*******************************************************************************
 **
 ** Function         btusb_isoc_check_msg
 **
 ** Description      Check the currently receiving message
 **
 ** Parameters       p_dev: device instance control block
 **                  pp_hdr: pointer to the receiving message
 **
 ** Returns          void
 **
 *******************************************************************************/
static void btusb_isoc_check_msg(tBTUSB_CB *p_dev, BT_HDR **pp_hdr)
{
    BT_HDR *p_hdr = *pp_hdr;
    unsigned char *p_data;

    // if enough data was received
    if (unlikely(p_hdr->len >= (1 + BTUSB_VOICE_HEADER_SIZE + SCO_RX_MAX_LEN)))
    {
        p_data = (void *)(p_hdr + 1);
        p_data[BTUSB_VOICE_HEADER_SIZE] = p_hdr->len - BTUSB_VOICE_HEADER_SIZE - 1;
        GKI_enqueue(&p_dev->rx_queue, p_hdr);

        // notify RX event(in case of select/poll
        wake_up_interruptible(&p_dev->rx_wait_q);

        *pp_hdr = NULL;
    }
}

/*******************************************************************************
 **
 ** Function         btusb_voicerx_complete
 **
 ** Description      Voice read (iso pipe) completion routine.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void btusb_voicerx_complete(struct urb *p_urb)
{
    tBTUSB_TRANSACTION *p_trans = p_urb->context;
    tBTUSB_CB *p_dev = p_trans->context;
    BT_HDR **pp_hdr, *p_hdr;
    unsigned int length, packet_length;
    unsigned char *p_packet, *p_frame, *p_data;
    struct usb_iso_packet_descriptor *p_uipd, *p_end;

    BTUSB_INFO("enter");

    btusb_voice_stats(&(p_dev->stats.voice_max_rx_rdy_delta_time), &(p_dev->stats.voice_min_rx_rdy_delta_time),
            &(p_dev->stats.voice_rx_rdy_delta_time), &(p_dev->stats.voice_last_rx_rdy_ts));

    p_dev->stats.voicerx_completed++;

    if (unlikely(!p_dev->p_main_intf || !p_dev->p_voice_in))
    {
        BTUSB_DBG("intf is down\n");
        return;
    }

    // entire URB error?
    if (unlikely(p_urb->status))
    {
        BTUSB_ERR("failure %d\n", p_urb->status);
        p_dev->stats.voicerx_completed_error++;
        return;
    }

    if (unlikely(p_dev->scosniff_active))
    {
        struct btusb_scosniff *bs;

        bs = kmalloc(sizeof(struct btusb_scosniff) +
                (p_urb->number_of_packets * sizeof(p_urb->iso_frame_desc[0])) +
                p_urb->transfer_buffer_length, GFP_ATOMIC);
        if (bs)
        {
            bs->s = p_urb->start_frame;
            bs->n = p_urb->number_of_packets;
            bs->l = p_urb->transfer_buffer_length;
            // copy the descriptors
            memcpy(bs->d, p_urb->iso_frame_desc, bs->n * sizeof(p_urb->iso_frame_desc[0]));
            // then copy the content of the buffer
            memcpy(&bs->d[bs->n], p_urb->transfer_buffer, bs->l);
            list_add_tail(&bs->lh, &p_dev->scosniff_list);
            complete(&p_dev->scosniff_completion);
        }
        else
        {
            BTUSB_ERR("Failed allocating scosniff buffer");
        }
    }

    p_frame = p_urb->transfer_buffer;
    p_end = &p_urb->iso_frame_desc[p_urb->number_of_packets];
    for (p_uipd = p_urb->iso_frame_desc; p_uipd < p_end; p_uipd++)
    {
        if (unlikely(p_uipd->status))
        {
            p_dev->stats.voicerx_bad_packets++;
            // should we do something if there is expected data?
            continue;
        }

        p_packet = p_frame + p_uipd->offset;
        packet_length = p_uipd->actual_length;
        p_dev->stats.voicerx_raw_bytes += packet_length;

        // waiting for data?
        if (likely(p_dev->pending_bytes))
        {
            fill_data:
            if (likely(p_dev->pending_bytes >= packet_length))
            {
                length = packet_length;
            }
            else
            {
                length = p_dev->pending_bytes;
            }
            pp_hdr = p_dev->pp_pending_msg;
            p_hdr = *pp_hdr;
            p_data = (void *)(p_hdr + 1) + p_hdr->len;
            // add data at the tail of the current message
            memcpy(p_data, p_packet, length);
            p_hdr->len += length;

            // decrement the number of bytes remaining
            p_dev->pending_bytes -= length;
            if (likely(p_dev->pending_bytes))
            {
                // data still needed -> next descriptor
                continue;
            }
            // no more pending bytes, check if it is full
            btusb_isoc_check_msg(p_dev, pp_hdr);
            packet_length -= length;
            if (likely(!packet_length))
                continue;
            // more bytes -> increment pointer
            p_packet += length;
        }

        // if there is still data in the packet
        if (likely(packet_length))
        {
            // at this point, there is NO SCO packet pending
            if (likely(packet_length >= (BTUSB_VOICE_HEADER_SIZE - p_dev->pending_hdr_size)))
            {
                length = BTUSB_VOICE_HEADER_SIZE - p_dev->pending_hdr_size;
            }
            else
            {
                length = packet_length;
            }

            // fill the hdr (in case header is split across descriptors)
            memcpy(&p_dev->pending_hdr[p_dev->pending_hdr_size], p_packet, length);
            p_dev->pending_hdr_size += length;

            if (likely(p_dev->pending_hdr_size == BTUSB_VOICE_HEADER_SIZE))
            {
                p_dev->pending_hdr_size = 0;

                if (likely(btusb_isoc_check_hdr(p_dev)))
                {
                    p_packet += length;
                    packet_length -= length;
                    // a correct header was found, get the data
                    goto fill_data;
                }
                p_dev->stats.voicerx_disc_nohdr++;
                p_dev->stats.voicerx_skipped_bytes += packet_length;
                // this is not a correct header -> next descriptor
                continue;
            }
            else if (likely(p_dev->pending_hdr_size < BTUSB_VOICE_HEADER_SIZE))
            {
                p_dev->stats.voicerx_split_hdr++;
                // header not complete -> next descriptor
                continue;
            }
            else
            {
                BTUSB_ERR("ISOC Header larger than possible. This is a major failure.\n");
                btusb_isoc_reset_msg(p_dev);
            }
        }
    }

    btusb_submit_voice_rx(p_dev, p_trans, GFP_ATOMIC);
}

