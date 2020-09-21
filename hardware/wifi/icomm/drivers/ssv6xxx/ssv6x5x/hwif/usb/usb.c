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
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/skbuff.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
#include <linux/printk.h>
#else
#include <linux/kernel.h>
#endif
#include <ssv6200.h>
#include <hwif/hwif.h>
#include "usb.h"
#include <ssv_data_struct.h>
#define USB_SSV_VENDOR_ID 0x8065
#define USB_SSV_PRODUCT_ID 0x6000
#define TRANSACTION_TIMEOUT (3000)
#define SSV6XXX_MAX_TXCMDSZ (sizeof(struct ssv6xxx_cmd_hdr))
#define SSV6XXX_MAX_RXCMDSZ (sizeof(struct ssv6xxx_cmd_hdr))
#define SSV6XXX_CMD_HEADER_SIZE (sizeof(struct ssv6xxx_cmd_hdr) - sizeof(union ssv6xxx_payload))
#define USB_CMD_SEQUENCE 255
#define MAX_RETRY_SSV6XXX_ALLOC_BUF 3
#define IS_GLUE_INVALID(glue) \
      ( (glue == NULL) \
       || (glue->dev_ready == false) \
       || ( (glue->p_wlan_data != NULL) \
           && (glue->p_wlan_data->is_enabled == false)) \
      )
static const struct usb_device_id ssv_usb_table[] = {
 { USB_DEVICE(USB_SSV_VENDOR_ID, USB_SSV_PRODUCT_ID) },
 { }
};
MODULE_DEVICE_TABLE(usb, ssv_usb_table);
extern int ssv_rx_nr_recvbuff;
extern int ssv_rx_use_wq;
struct ssv6xxx_usb_glue {
 struct device *dev;
 struct platform_device *core;
 struct usb_device *udev;
 struct usb_interface *interface;
    struct ssv6xxx_platform_data *p_wlan_data;
 struct ssv6xxx_platform_data tmp_data;
 struct ssv6xxx_cmd_endpoint cmd_endpoint;
 struct ssv6xxx_cmd_endpoint rsp_endpoint;
 struct ssv6xxx_tx_endpoint tx_endpoint;
 struct ssv6xxx_rx_endpoint rx_endpoint;
 struct ssv6xxx_rx_buf ssv_rx_buf[MAX_NR_RECVBUFF];
 struct ssv6xxx_queue ssv_rx_queue;
 struct kref kref;
 struct mutex io_mutex;
 struct mutex cmd_mutex;
 u16 sequence;
 u16 err_cnt;
 bool dev_ready;
 struct workqueue_struct *wq;
 struct ssv6xxx_usb_work_struct rx_work;
 struct tasklet_struct rx_tasklet;
 u32 *rx_pkt;
 void *rx_cb_args;
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
    int (*rx_cb)(struct sk_buff_head *rxq, void *args);
#else
    int (*rx_cb)(struct sk_buff *rx_skb, void *args);
#endif
    int (*is_rx_q_full)(void *);
};
static void ssv6xxx_usb_recv_rx(struct ssv6xxx_usb_glue *glue, struct ssv6xxx_rx_buf *ssv_rx_buf);
#define to_ssv6xxx_usb_dev(d) container_of(d, struct ssv6xxx_usb_glue, kref)
static struct usb_driver ssv_usb_driver;
#if 0
static void ssv6xxx_dump_tx_desc(const u8 *buf)
{
    struct ssv6200_tx_desc *tx_desc;
    tx_desc = (struct ssv6200_tx_desc *)buf;
    printk(">> Tx Frame:\n");
    printk("length: %d, c_type=%d, f80211=%d, qos=%d, ht=%d, use_4addr=%d, sec=%d\n",
        tx_desc->len, tx_desc->c_type, tx_desc->f80211, tx_desc->qos, tx_desc->ht,
        tx_desc->use_4addr, tx_desc->security);
    printk("more_data=%d, sub_type=%x, extra_info=%d\n", tx_desc->more_data,
        tx_desc->stype_b5b4, tx_desc->extra_info);
    printk("fcmd=0x%08x, hdr_offset=%d, frag=%d, unicast=%d, hdr_len=%d\n",
        tx_desc->fCmd, tx_desc->hdr_offset, tx_desc->frag, tx_desc->unicast,
        tx_desc->hdr_len);
    printk("tx_burst=%d, ack_policy=%d, do_rts_cts=%d, reason=%d, payload_offset=%d\n",
        tx_desc->tx_burst, tx_desc->ack_policy, tx_desc->do_rts_cts,
        tx_desc->reason, tx_desc->payload_offset);
    printk("fcmdidx=%d, wsid=%d, txq_idx=%d\n",
         tx_desc->fCmdIdx, tx_desc->wsid, tx_desc->txq_idx);
    printk("RTS/CTS Nav=%d, frame_time=%d, crate_idx=%d, drate_idx=%d, dl_len=%d\n",
        tx_desc->rts_cts_nav, tx_desc->frame_consume_time, tx_desc->crate_idx, tx_desc->drate_idx,
        tx_desc->dl_length);
 printk("\n\n\n");
}
#endif
#if 0
static void ssv6xxx_dump_rx_desc(const u8 *buf)
{
    struct ssv6200_rx_desc *rx_desc;
    rx_desc = (struct ssv6200_rx_desc *)buf;
    printk(">> RX Descriptor:\n");
    printk("len=%d, c_type=%d, f80211=%d, qos=%d, ht=%d, use_4addr=%d, l3cs_err=%d, l4_cs_err=%d\n",
        rx_desc->len, rx_desc->c_type, rx_desc->f80211, rx_desc->qos, rx_desc->ht, rx_desc->use_4addr,
        rx_desc->l3cs_err, rx_desc->l4cs_err);
    printk("align2=%d, psm=%d, stype_b5b4=%d, extra_info=%d\n",
        rx_desc->align2, rx_desc->psm, rx_desc->stype_b5b4, rx_desc->extra_info);
    printk("hdr_offset=%d, reason=%d, rx_result=%d\n", rx_desc->hdr_offset,
        rx_desc->reason, rx_desc->RxResult);
 printk("\n\n\n");
}
#endif
static u16 ssv6xxx_get_cmd_sequence(struct ssv6xxx_usb_glue *glue)
{
 glue->sequence = glue->sequence % USB_CMD_SEQUENCE;
 (glue->sequence)++;
 return glue->sequence;
}
static void ssv6xxx_usb_delete(struct kref *kref)
{
 struct ssv6xxx_usb_glue *glue = to_ssv6xxx_usb_dev(kref);
 int i;
 for (i = 0 ; i < MAX_NR_RECVBUFF ; ++i) {
  usb_kill_urb(glue->ssv_rx_buf[i].rx_urb);
 }
 if (glue->cmd_endpoint.buff)
  kfree(glue->cmd_endpoint.buff);
 if (glue->rsp_endpoint.buff)
  kfree(glue->rsp_endpoint.buff);
 if (glue->ssv_rx_buf[0].rx_buf) {
  for (i = 0 ; i < MAX_NR_RECVBUFF ; ++i) {
   usb_free_coherent(glue->udev, MAX_HCI_RX_AGGR_SIZE,
    glue->ssv_rx_buf[i].rx_buf,
    glue->ssv_rx_buf[i].rx_urb->transfer_dma);
   usb_free_urb(glue->ssv_rx_buf[i].rx_urb);
  }
 }
 if (ssv_rx_use_wq) {
  destroy_workqueue(glue->wq);
 } else {
  tasklet_kill(&glue->rx_tasklet);
 }
 usb_put_dev(glue->udev);
 kfree(glue);
}
static int ssv6xxx_usb_recv_rsp(struct ssv6xxx_usb_glue *glue, int size, int *rsp_len)
{
 int retval = 0, foolen = 0;
 if (!glue || !glue->interface) {
  retval = -ENODEV;
  return retval;
 }
 retval = usb_bulk_msg(glue->udev,
     usb_rcvbulkpipe(glue->udev, glue->rsp_endpoint.address),
     glue->rsp_endpoint.buff, size,
     &foolen, TRANSACTION_TIMEOUT);
 if (retval) {
  *rsp_len = 0;
  HWIF_DBG_PRINT(glue->p_wlan_data, "Cannot receive response, error=%d\n", retval);
 } else {
  *rsp_len = foolen;
  glue->err_cnt = 0;
 }
 return retval;
}
static int ssv6xxx_usb_send_cmd(struct ssv6xxx_usb_glue *glue, u8 cmd, u16 seq, const void *data, u32 data_len)
{
 int retval = 0, foolen = 0;
 struct ssv6xxx_cmd_hdr *hdr;
 if (!glue || !glue->interface) {
  retval = -ENODEV;
  return retval;
 }
 hdr = (struct ssv6xxx_cmd_hdr *)glue->cmd_endpoint.buff;
 memset(hdr, 0, sizeof(struct ssv6xxx_cmd_hdr));
 hdr->plen = (data_len >> (0))& 0xff;
 hdr->cmd = cmd;
 hdr->seq = cpu_to_le16(seq);
 memcpy(&hdr->payload, data, data_len);
 retval = usb_bulk_msg(glue->udev,
     usb_sndbulkpipe(glue->udev, glue->cmd_endpoint.address),
     glue->cmd_endpoint.buff, (data_len+SSV6XXX_CMD_HEADER_SIZE),
     &foolen, TRANSACTION_TIMEOUT);
 if (retval) {
  HWIF_DBG_PRINT(glue->p_wlan_data, "Cannot send cmd data, error=%d\n", retval);
 } else {
  glue->err_cnt = 0;
 }
 return retval;
}
static int ssv6xxx_usb_cmd(struct ssv6xxx_usb_glue *glue, u8 cmd, void *data, u32 data_len, void *result)
{
 int retval = (-1), rsp_len = 0, i = 0;
 struct ssv6xxx_cmd_hdr *rsphdr;
 u16 sequence;
 mutex_lock(&glue->cmd_mutex);
 sequence = ssv6xxx_get_cmd_sequence(glue);
 retval = ssv6xxx_usb_send_cmd(glue, cmd, sequence, data, data_len);
 if (retval) {
  HWIF_DBG_PRINT(glue->p_wlan_data, "%s: Fail to send cmd, sequence=%d, retval=%d\n",
    __FUNCTION__, sequence, retval);
  goto exit;
 }
 for (i = 0; i < USB_CMD_SEQUENCE; i++) {
  retval = ssv6xxx_usb_recv_rsp(glue, SSV6XXX_MAX_RXCMDSZ, &rsp_len);
  if (retval) {
   HWIF_DBG_PRINT(glue->p_wlan_data, "%s: Fail to receive response, sequence=%d, retval=%d\n",
     __FUNCTION__, sequence, retval);
   goto exit;
  }
  if (rsp_len < SSV6XXX_CMD_HEADER_SIZE) {
   HWIF_DBG_PRINT(glue->p_wlan_data, "Receviced abnormal response length[%d]\n", rsp_len);
   goto exit;
  }
  rsphdr = (struct ssv6xxx_cmd_hdr *)glue->rsp_endpoint.buff;
  if (sequence == rsphdr->seq)
   break;
  else
   HWIF_DBG_PRINT(glue->p_wlan_data, "received incorrect sequence=%d[%d]\n", sequence, rsphdr->seq);
 }
 switch (rsphdr->cmd) {
  case SSV6200_CMD_WRITE_REG:
   break;
  case SSV6200_CMD_READ_REG:
   if (result)
    memcpy(result, &rsphdr->payload, sizeof(struct ssv6xxx_read_reg_result));
   break;
  default:
   retval = -1;
   HWIF_DBG_PRINT(glue->p_wlan_data, "%s: unknown response cmd[%d]\n", __FUNCTION__, rsphdr->cmd);
   break;
 }
exit:
 mutex_unlock(&glue->cmd_mutex);
 return retval;
}
static void ssv6xxx_usb_recv_rx_work(struct work_struct *work)
{
    struct ssv6xxx_usb_glue *glue = ((struct ssv6xxx_usb_work_struct *)work)->glue;
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
    struct sk_buff_head rx_list;
#endif
    struct sk_buff *rx_mpdu;
    struct ssv6xxx_rx_buf *ssv_rx_buf;
    unsigned char *data;
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
    skb_queue_head_init(&rx_list);
#endif
    while (NULL != (ssv_rx_buf = (struct ssv6xxx_rx_buf *)ssv6xxx_dequeue_list_node(&glue->ssv_rx_queue))) {
        if (glue->is_rx_q_full(glue->rx_cb_args)) {
            ssv6xxx_enqueue_list_node((struct ssv6xxx_list_node *)ssv_rx_buf, &glue->ssv_rx_queue);
            queue_work(glue->wq, (struct work_struct *)&glue->rx_work);
            break;
        }
        (*glue->rx_pkt)++;
        rx_mpdu = glue->p_wlan_data->skb_alloc(glue->p_wlan_data->skb_param, ssv_rx_buf->rx_filled,
  GFP_KERNEL
  );
        if (rx_mpdu == NULL) {
            ssv6xxx_enqueue_list_node((struct ssv6xxx_list_node *)ssv_rx_buf, &glue->ssv_rx_queue);
            queue_work(glue->wq, (struct work_struct *)&glue->rx_work);
            break;
        }
        data = skb_put(rx_mpdu, ssv_rx_buf->rx_filled);
        memcpy(data, ssv_rx_buf->rx_buf, ssv_rx_buf->rx_filled);
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
        skb_queue_tail(&rx_list, rx_mpdu);
#else
        glue->rx_cb(rx_mpdu, glue->rx_cb_args);
#endif
        ssv6xxx_usb_recv_rx(glue, ssv_rx_buf);
    }
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
    if (skb_queue_len(&rx_list)) {
        glue->rx_cb(&rx_list, glue->rx_cb_args);
    }
#endif
}
static void ssv6xxx_usb_recv_rx_tasklet(unsigned long priv)
{
    struct ssv6xxx_usb_glue *glue = (struct ssv6xxx_usb_glue *)priv;
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
    struct sk_buff_head rx_list;
#endif
    struct sk_buff *rx_mpdu;
    struct ssv6xxx_rx_buf *ssv_rx_buf;
    unsigned char *data;
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
    skb_queue_head_init(&rx_list);
#endif
    while (NULL != (ssv_rx_buf = (struct ssv6xxx_rx_buf *)ssv6xxx_dequeue_list_node(&glue->ssv_rx_queue))) {
        if (glue->is_rx_q_full(glue->rx_cb_args)) {
            ssv6xxx_enqueue_list_node((struct ssv6xxx_list_node *)ssv_rx_buf, &glue->ssv_rx_queue);
            tasklet_schedule(&glue->rx_tasklet);
            break;
        }
        (*glue->rx_pkt)++;
        rx_mpdu = glue->p_wlan_data->skb_alloc(glue->p_wlan_data->skb_param, ssv_rx_buf->rx_filled,
  GFP_ATOMIC
  );
        if (rx_mpdu == NULL) {
            ssv6xxx_enqueue_list_node((struct ssv6xxx_list_node *)ssv_rx_buf, &glue->ssv_rx_queue);
            tasklet_schedule(&glue->rx_tasklet);
            break;
        }
        data = skb_put(rx_mpdu, ssv_rx_buf->rx_filled);
        memcpy(data, ssv_rx_buf->rx_buf, ssv_rx_buf->rx_filled);
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
        skb_queue_tail(&rx_list, rx_mpdu);
#else
        glue->rx_cb(rx_mpdu, glue->rx_cb_args);
#endif
        ssv6xxx_usb_recv_rx(glue, ssv_rx_buf);
    }
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
    if (skb_queue_len(&rx_list)) {
        glue->rx_cb(&rx_list, glue->rx_cb_args);
    }
#endif
}
static void ssv6xxx_usb_recv_rx_complete(struct urb *urb)
{
    struct ssv6xxx_rx_buf *ssv_rx_buf = (struct ssv6xxx_rx_buf *)urb->context;
    struct ssv6xxx_usb_glue *glue = ssv_rx_buf->glue;
    ssv_rx_buf->rx_res = urb->status;
    if (urb->status) {
        HWIF_DBG_PRINT(glue->p_wlan_data, "fail rx status received:%d\n", urb->status);
        goto skip;
    }
    glue->err_cnt = 0;
    ssv_rx_buf->rx_filled = urb->actual_length;
    if (ssv_rx_buf->rx_filled > MAX_HCI_RX_AGGR_SIZE) {
        HWIF_DBG_PRINT(glue->p_wlan_data, "recv invalid data length %d\n", ssv_rx_buf->rx_filled);
        goto skip;
    }
    ssv6xxx_enqueue_list_node((struct ssv6xxx_list_node *)ssv_rx_buf, &glue->ssv_rx_queue);
    if (ssv_rx_use_wq) {
        queue_work(glue->wq, (struct work_struct *)&glue->rx_work);
    } else {
        tasklet_schedule(&glue->rx_tasklet);
    }
    return;
skip:
    ssv6xxx_usb_recv_rx(glue, ssv_rx_buf);
}
static void ssv6xxx_usb_recv_rx(struct ssv6xxx_usb_glue *glue, struct ssv6xxx_rx_buf *ssv_rx_buf)
{
 int size = MAX_HCI_RX_AGGR_SIZE;
 int retval;
 usb_fill_bulk_urb(ssv_rx_buf->rx_urb,
   glue->udev, usb_rcvbulkpipe(glue->udev, glue->rx_endpoint.address),
   ssv_rx_buf->rx_buf, size,
   ssv6xxx_usb_recv_rx_complete, ssv_rx_buf);
 ssv_rx_buf->rx_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
 ssv_rx_buf->rx_filled = 0;
 retval = usb_submit_urb(ssv_rx_buf->rx_urb, GFP_ATOMIC);
 if (retval) {
  HWIF_DBG_PRINT(glue->p_wlan_data, "Fail to submit rx urb, error=%d\n", retval);
 }
}
static int __must_check ssv6xxx_usb_read(struct device *child,
        void *buf, size_t *size, int mode)
{
 *size = 0;
 return 0;
}
static int ssv6xxx_usb_send_tx(struct ssv6xxx_usb_glue *glue, struct sk_buff *skb, size_t size)
{
 int foolen = 0, retval = 0;
 int tx_len = size;
 if ((tx_len % glue->tx_endpoint.packet_size) == 0) {
  skb_put(skb, 1);
  tx_len++;
 }
 retval = usb_bulk_msg(glue->udev,
     usb_sndbulkpipe(glue->udev, glue->tx_endpoint.address),
     skb->data, tx_len, &foolen, TRANSACTION_TIMEOUT);
 if (retval)
  HWIF_DBG_PRINT(glue->p_wlan_data, "Cannot send tx data, retval=%d\n", retval);
 return retval;
}
static int __must_check ssv6xxx_usb_write(struct device *child,
        void *buf, size_t len, u8 queue_num)
{
    int retval = (-1);
    struct ssv6xxx_usb_glue *glue = dev_get_drvdata(child->parent);
    if (IS_GLUE_INVALID(glue))
        return retval;
    if ((retval = ssv6xxx_usb_send_tx(glue, (struct sk_buff *)buf, len)) < 0) {
  HWIF_DBG_PRINT(glue->p_wlan_data, "%s: Fail to send tx data\n", __FUNCTION__);
 } else {
  glue->err_cnt = 0;
 }
 return retval;
}
static int __must_check __ssv6xxx_usb_read_reg(struct ssv6xxx_usb_glue *glue, u32 addr,
  u32 *buf)
{
    int retval = (-1);
 struct ssv6xxx_read_reg read_reg;
 struct ssv6xxx_read_reg_result result;
 if (IS_GLUE_INVALID(glue))
  return retval;
 memset(&read_reg, 0, sizeof(struct ssv6xxx_read_reg));
 memset(&result, 0, sizeof(struct ssv6xxx_read_reg_result));
 read_reg.addr = cpu_to_le32(addr);
 retval = ssv6xxx_usb_cmd(glue, SSV6200_CMD_READ_REG, &read_reg, sizeof(struct ssv6xxx_read_reg), &result);
 if (!retval)
  *buf = le32_to_cpu(result.value);
 else {
  *buf = 0xffffffff;
  HWIF_DBG_PRINT(glue->p_wlan_data, "%s: Fail to read register address %x\n", __FUNCTION__, addr);
 }
 return retval;
}
static int __must_check ssv6xxx_usb_read_reg(struct device *child, u32 addr,
                                             u32 *buf)
{
    struct ssv6xxx_usb_glue *glue = dev_get_drvdata(child->parent);
    return __ssv6xxx_usb_read_reg(glue, addr, buf);
}
static int __must_check __ssv6xxx_usb_write_reg(struct ssv6xxx_usb_glue *glue, u32 addr,
   u32 buf)
{
 int retval = (-1);
 struct ssv6xxx_write_reg write_reg;
 if (IS_GLUE_INVALID(glue))
  return retval;
 memset(&write_reg, 0, sizeof(struct ssv6xxx_write_reg));
 write_reg.addr = cpu_to_le32(addr);
 write_reg.value = cpu_to_le32(buf);
 retval = ssv6xxx_usb_cmd(glue, SSV6200_CMD_WRITE_REG, &write_reg, sizeof(struct ssv6xxx_write_reg), NULL);
 if (retval)
  HWIF_DBG_PRINT(glue->p_wlan_data, "%s: Fail to write register address %x, value %x\n", __FUNCTION__, addr, buf);
 return retval;
}
static int __must_check ssv6xxx_usb_write_reg(struct device *child, u32 addr,
  u32 buf)
{
    struct ssv6xxx_usb_glue *glue = dev_get_drvdata(child->parent);
    return __ssv6xxx_usb_write_reg(glue, addr, buf);
}
static int __must_check ssv6xxx_usb_burst_read_reg(struct device *child, u32 *addr,
        u32 *buf, u8 reg_amount)
{
    int ret = -1, i;
    printk("%s(): Not support atomic burst register reading!\n", __func__);
    WARN_ON(1);
    for (i = 0 ; i < reg_amount ; i++) {
        ret = ssv6xxx_usb_read_reg(child, addr[i], &buf[i]);
        if (ret != 0) {
            printk("%s(): read 0x%08x failed.\n", __func__, addr[i]);
        }
    }
    return -EOPNOTSUPP;
}
static int __must_check ssv6xxx_usb_burst_write_reg(struct device *child, u32 *addr,
        u32 *buf, u8 reg_amount)
{
    int ret = -1, i;
    printk("%s(): Not support atomic burst register writing!\n", __func__);
    WARN_ON(1);
    for (i = 0 ; i < reg_amount ; i++) {
        ret = ssv6xxx_usb_write_reg(child, addr[i], buf[i]);
        if (ret != 0) {
            printk("%s(): write 0x%08x failed.\n", __func__, addr[i]);
        }
    }
    return -EOPNOTSUPP;
}
static int ssv6xxx_usb_load_firmware(struct device *child, u32 start_addr, u8 *data, int data_length)
{
 struct ssv6xxx_usb_glue *glue = dev_get_drvdata(child->parent);
    u16 laddr, haddr;
 u32 addr;
 int retval = 0, max_usb_block = 512;
 u8 *pdata;
 int res_length, offset, send_length;
 if (IS_GLUE_INVALID(glue))
  return -1;
 offset = 0;
 pdata = data;
 addr = start_addr;
 res_length = data_length;
 while (offset < data_length) {
  int transfer = min_t(int, res_length, max_usb_block);
  laddr = (addr & 0x0000ffff);
  haddr = (addr >> 16);
  send_length = usb_control_msg(glue->udev, usb_sndctrlpipe(glue->udev, 0),
   FIRMWARE_DOWNLOAD, (USB_DIR_OUT | USB_TYPE_VENDOR),
   laddr, haddr, pdata, transfer, TRANSACTION_TIMEOUT);
  if (send_length < 0) {
   retval = send_length;
   HWIF_DBG_PRINT(glue->p_wlan_data, "Load Firmware Fail, retval=%d, sram=0x%08x\n", retval, (laddr|haddr));
   break;
  }
  addr += transfer;
  pdata += transfer;
  offset += transfer;
  res_length -= transfer;
 }
 return retval;
}
static int ssv6xxx_usb_property(struct device *child)
{
 return SSV_HWIF_CAPABILITY_POLLING | SSV_HWIF_INTERFACE_USB;
}
static int ssv6xxx_chk_usb_speed(struct ssv6xxx_usb_glue *glue)
{
    if (IS_GLUE_INVALID(glue)) {
        return -1;
    }
    return glue->udev->speed;
}
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
static void ssv6xxx_usb_rx_task(struct device *child,
   int (*rx_cb)(struct sk_buff_head *rxq, void *args),
   int (*is_rx_q_full)(void *args), void *args, u32 *pkt)
#else
static void ssv6xxx_usb_rx_task(struct device *child,
   int (*rx_cb)(struct sk_buff *rx_skb, void *args),
   int (*is_rx_q_full)(void *args), void *args, u32 *pkt)
#endif
{
 struct ssv6xxx_usb_glue *glue = dev_get_drvdata(child->parent);
 int i;
 int nr_recvbuff = (ssv_rx_nr_recvbuff > MAX_NR_RECVBUFF)?MAX_NR_RECVBUFF:((ssv_rx_nr_recvbuff < MIN_NR_RECVBUFF)?MIN_NR_RECVBUFF:ssv_rx_nr_recvbuff);
 printk("%s: nr_recvbuff=%d\n", __func__, nr_recvbuff);
 glue->rx_cb = rx_cb;
 glue->rx_cb_args = args;
 glue->is_rx_q_full = is_rx_q_full;
 glue->rx_pkt = pkt;
 for (i = 0 ; i < nr_recvbuff ; ++i) {
  ssv6xxx_usb_recv_rx(glue, &(glue->ssv_rx_buf[i]));
 }
}
static int ssv6xxx_usb_start_acc(struct device *child, u8 epnum)
{
    struct ssv6xxx_usb_glue *glue = dev_get_drvdata(child->parent);
    if (IS_GLUE_INVALID(glue)) {
        printk("failed to start usb acc of ep%d\n", epnum);
        return -1;
    }
    if (ssv6xxx_chk_usb_speed(glue) == USB_SPEED_HIGH)
        glue->p_wlan_data->enable_usb_acc(glue->p_wlan_data->usb_param, epnum);
    return 0;
}
static int ssv6xxx_usb_stop_acc(struct device *child, u8 epnum)
{
    struct ssv6xxx_usb_glue *glue = dev_get_drvdata(child->parent);
    if (IS_GLUE_INVALID(glue)) {
        printk("failed to stop usb acc of ep%d\n", epnum);
        return -1;
    }
    if (ssv6xxx_chk_usb_speed(glue) == USB_SPEED_HIGH)
        glue->p_wlan_data->disable_usb_acc(glue->p_wlan_data->usb_param, epnum);
    return 0;
}
static int ssv6xxx_usb_jump_to_rom(struct device *child)
{
    struct ssv6xxx_usb_glue *glue = dev_get_drvdata(child->parent);
    if (IS_GLUE_INVALID(glue)) {
        printk("failed to jump to ROM\n");
        return -1;
    }
    glue->p_wlan_data->jump_to_rom(glue->p_wlan_data->usb_param);
    return 0;
}
static void ssv6xxx_usb_sysplf_reset(struct device *child, u32 addr, u32 value)
{
    struct ssv6xxx_usb_glue *glue = dev_get_drvdata(child->parent);
 int retval = (-1), rsp_len = 0;
 u16 sequence;
 struct ssv6xxx_write_reg write_reg;
    if (IS_GLUE_INVALID(glue))
        return;
 mutex_lock(&glue->cmd_mutex);
 sequence = ssv6xxx_get_cmd_sequence(glue);
 memset(&write_reg, 0, sizeof(struct ssv6xxx_write_reg));
 write_reg.addr = cpu_to_le32(addr);
 write_reg.value = cpu_to_le32(value);
 retval = ssv6xxx_usb_send_cmd(glue, SSV6200_CMD_WRITE_REG, sequence, &write_reg, sizeof(struct ssv6xxx_write_reg));
 if (retval)
     HWIF_DBG_PRINT(glue->p_wlan_data, "%s: Fail to reset sysplf\n", __FUNCTION__);
    retval = ssv6xxx_usb_recv_rsp(glue, SSV6XXX_MAX_RXCMDSZ, &rsp_len);
    mutex_unlock(&glue->cmd_mutex);
}
static struct ssv6xxx_hwif_ops usb_ops =
{
    .read = ssv6xxx_usb_read,
    .write = ssv6xxx_usb_write,
    .readreg = ssv6xxx_usb_read_reg,
    .writereg = ssv6xxx_usb_write_reg,
    .safe_readreg = ssv6xxx_usb_read_reg,
    .safe_writereg = ssv6xxx_usb_write_reg,
    .burst_readreg = ssv6xxx_usb_burst_read_reg,
    .burst_writereg = ssv6xxx_usb_burst_write_reg,
    .burst_safe_readreg = ssv6xxx_usb_burst_read_reg,
    .burst_safe_writereg = ssv6xxx_usb_burst_write_reg,
    .load_fw = ssv6xxx_usb_load_firmware,
    .property = ssv6xxx_usb_property,
    .hwif_rx_task = ssv6xxx_usb_rx_task,
    .start_usb_acc = ssv6xxx_usb_start_acc,
    .stop_usb_acc = ssv6xxx_usb_stop_acc,
    .jump_to_rom = ssv6xxx_usb_jump_to_rom,
 .sysplf_reset = ssv6xxx_usb_sysplf_reset,
};
static void ssv6xxx_usb_power_on(struct ssv6xxx_platform_data * pdata, struct usb_interface *interface)
{
 if (pdata->is_enabled == true)
  return;
 pdata->is_enabled = true;
}
static void ssv6xxx_usb_power_off(struct ssv6xxx_platform_data * pdata, struct usb_interface *interface)
{
 if (pdata->is_enabled == false)
  return;
 pdata->is_enabled = false;
}
static void _read_chip_id (struct ssv6xxx_usb_glue *glue)
{
    u32 regval;
    int ret;
    u8 _chip_id[SSV6XXX_CHIP_ID_LENGTH];
    u8 *c = _chip_id;
    int i = 0;
    ret = __ssv6xxx_usb_read_reg(glue, ADR_CHIP_ID_3, &regval);
    *((u32 *)&_chip_id[0]) = __be32_to_cpu(regval);
    if (ret == 0)
        ret = __ssv6xxx_usb_read_reg(glue, ADR_CHIP_ID_2, &regval);
    *((u32 *)&_chip_id[4]) = __be32_to_cpu(regval);
    if (ret == 0)
        ret = __ssv6xxx_usb_read_reg(glue, ADR_CHIP_ID_1, &regval);
    *((u32 *)&_chip_id[8]) = __be32_to_cpu(regval);
    if (ret == 0)
        ret = __ssv6xxx_usb_read_reg(glue, ADR_CHIP_ID_0, &regval);
    *((u32 *)&_chip_id[12]) = __be32_to_cpu(regval);
    _chip_id[12+sizeof(u32)] = 0;
    while (*c == 0) {
        i++;
        c++;
        if (i == 16) {
            c = _chip_id;
            break;
        }
    }
    if (*c != 0) {
        strncpy(glue->tmp_data.chip_id, c, SSV6XXX_CHIP_ID_LENGTH);
        dev_info(glue->dev, "CHIP ID: %s \n", glue->tmp_data.chip_id);
        strncpy(glue->tmp_data.short_chip_id, c, SSV6XXX_CHIP_ID_SHORT_LENGTH);
        glue->tmp_data.short_chip_id[SSV6XXX_CHIP_ID_SHORT_LENGTH] = 0;
    } else {
        dev_err(glue->dev, "Failed to read chip ID");
        glue->tmp_data.chip_id[0] = 0;
    }
}
static int ssv_usb_probe(struct usb_interface *interface,
        const struct usb_device_id *id)
{
 struct ssv6xxx_platform_data *pwlan_data;
 struct ssv6xxx_usb_glue *glue;
 struct usb_host_interface *iface_desc;
 struct usb_endpoint_descriptor *endpoint;
 int i, j;
 int retval = -ENOMEM;
 unsigned int epnum;
 printk(KERN_INFO "=======================================\n");
 printk(KERN_INFO "==          TURISMO - USB            ==\n");
 printk(KERN_INFO "=======================================\n");
 glue = kzalloc(sizeof(*glue), GFP_KERNEL);
 if (!glue) {
  dev_err(&interface->dev, "Out of memory\n");
  goto error;
 }
 glue->sequence = 0;
 glue->err_cnt = 0;
 kref_init(&glue->kref);
 mutex_init(&glue->io_mutex);
 mutex_init(&glue->cmd_mutex);
 tu_ssv6xxx_init_queue(&glue->ssv_rx_queue);
 if (ssv_rx_use_wq) {
  glue->rx_work.glue = glue;
  INIT_WORK((struct work_struct *)&glue->rx_work, ssv6xxx_usb_recv_rx_work);
  glue->wq = create_singlethread_workqueue("ssv6xxx_usb_wq");
  if (!glue->wq) {
   dev_err(&interface->dev, "Could not allocate Work Queue\n");
   goto error;
  }
 } else {
  tasklet_init(&glue->rx_tasklet, ssv6xxx_usb_recv_rx_tasklet, (unsigned long)glue);
 }
 pwlan_data = &glue->tmp_data;
 memset(pwlan_data, 0, sizeof(struct ssv6xxx_platform_data));
 atomic_set(&pwlan_data->irq_handling, 0);
 glue->dev = &interface->dev;
 glue->udev = usb_get_dev(interface_to_usbdev(interface));
 glue->interface = interface;
 glue->dev_ready = true;
 pwlan_data->vendor = id->idVendor;
 pwlan_data->device = id->idProduct;
 pwlan_data->ops = &usb_ops;
 iface_desc = interface->cur_altsetting;
 for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
  endpoint = &iface_desc->endpoint[i].desc;
  epnum = endpoint->bEndpointAddress & 0x0f;
  if (epnum == SSV_EP_CMD) {
   glue->cmd_endpoint.address = endpoint->bEndpointAddress;
   glue->cmd_endpoint.packet_size = le16_to_cpu(endpoint->wMaxPacketSize);
   glue->cmd_endpoint.buff = kmalloc(SSV6XXX_MAX_TXCMDSZ, GFP_ATOMIC);
   if (!glue->cmd_endpoint.buff) {
    dev_err(&interface->dev, "Could not allocate cmd buffer\n");
    goto error;
   }
  }
  if (epnum == SSV_EP_RSP) {
   glue->rsp_endpoint.address = endpoint->bEndpointAddress;
   glue->rsp_endpoint.packet_size = le16_to_cpu(endpoint->wMaxPacketSize);
   glue->rsp_endpoint.buff = kmalloc(SSV6XXX_MAX_RXCMDSZ, GFP_ATOMIC);
   if (!glue->rsp_endpoint.buff) {
    dev_err(&interface->dev, "Could not allocate rsp buffer\n");
    goto error;
   }
  }
  if (epnum == SSV_EP_TX) {
   glue->tx_endpoint.address = endpoint->bEndpointAddress;
   glue->tx_endpoint.packet_size = le16_to_cpu(endpoint->wMaxPacketSize);
        }
  if (epnum == SSV_EP_RX) {
   glue->rx_endpoint.address = endpoint->bEndpointAddress;
   glue->rx_endpoint.packet_size = le16_to_cpu(endpoint->wMaxPacketSize);
   for (j = 0 ; j < MAX_NR_RECVBUFF ; ++j) {
    glue->ssv_rx_buf[j].rx_urb = usb_alloc_urb(0, GFP_ATOMIC);
    if (!glue->ssv_rx_buf[j].rx_urb) {
     dev_err(&interface->dev, "Could not allocate rx urb\n");
     goto error;
    }
    glue->ssv_rx_buf[j].rx_buf = usb_alloc_coherent(
     glue->udev, MAX_HCI_RX_AGGR_SIZE,
     GFP_ATOMIC, &glue->ssv_rx_buf[j].rx_urb->transfer_dma);
    if (!glue->ssv_rx_buf[j].rx_buf) {
     dev_err(&interface->dev, "Could not allocate rx buffer\n");
     goto error;
    }
    glue->ssv_rx_buf[j].glue = glue;
    tu_ssv6xxx_init_list_node((struct ssv6xxx_list_node *)&glue->ssv_rx_buf[j]);
   }
  }
 }
 if (!(glue->cmd_endpoint.address &&
    glue->rsp_endpoint.address &&
    glue->tx_endpoint.address &&
    glue->rx_endpoint.address))
 {
  dev_err(&interface->dev, "Could not find all endpoints\n");
  goto error;
 }
 usb_set_intfdata(interface, glue);
 ssv6xxx_usb_power_on(pwlan_data, interface);
 _read_chip_id(glue);
 glue->core = platform_device_alloc(pwlan_data->short_chip_id, -1);
 if (!glue->core) {
  dev_err(glue->dev, "can't allocate platform_device");
  retval = -ENOMEM;
  goto error;
 }
 glue->core->dev.parent = &interface->dev;
 retval = platform_device_add_data(glue->core, pwlan_data, sizeof(*pwlan_data));
 if (retval) {
  dev_err(glue->dev, "can't add platform data\n");
  goto out_dev_put;
 }
    glue->p_wlan_data = glue->core->dev.platform_data;
 retval = platform_device_add(glue->core);
 if (retval) {
  dev_err(glue->dev, "can't add platform device\n");
  goto out_dev_put;
 }
#ifdef SSV_SUPPORT_USB_LPM
    printk("---------- USB LPM capability ---------- \n");
    printk("device supports LPM: %d\n", glue->udev->lpm_capable);
    printk("device can perform USB2 hardware LPM: %d\n", glue->udev->usb2_hw_lpm_capable);
    printk("USB2 (Host) hardware LPM is enabled: %d\n", glue->udev->usb2_hw_lpm_enabled);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,12,2)
    printk("Userspace allows USB 2.0 LPM to be enabled: %d\n", glue->udev->usb2_hw_lpm_allowed);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,11,0)
    printk("device can perform USB2 hardware BESL LPM: %d\n", glue->udev->usb2_hw_lpm_besl_capable);
#endif
    printk("----------------------------------------\n");
#endif
 return 0;
out_dev_put:
 platform_device_put(glue->core);
error:
 if (glue)
  kref_put(&glue->kref, ssv6xxx_usb_delete);
 return retval;
}
static void ssv_usb_disconnect(struct usb_interface *interface)
{
 struct ssv6xxx_usb_glue *glue;
 glue = usb_get_intfdata(interface);
 usb_set_intfdata(interface, NULL);
 if (glue) {
  glue->dev_ready = false;
  ssv6xxx_usb_power_off(glue->p_wlan_data, interface);
  printk("platform_device_del \n");
  platform_device_del(glue->core);
  printk("platform_device_put \n");
  platform_device_put(glue->core);
 }
 mutex_lock(&glue->io_mutex);
 glue->interface = NULL;
 mutex_unlock(&glue->io_mutex);
 kref_put(&glue->kref, ssv6xxx_usb_delete);
 dev_info(&interface->dev, "SSV USB is disconnected");
}
#ifdef CONFIG_PM
static int ssv_usb_suspend(struct usb_interface *interface, pm_message_t message)
{
 struct ssv6xxx_usb_glue *glue = usb_get_intfdata(interface);
 int i;
 dev_info(glue->dev, "%s(): suspend.\n", __FUNCTION__);
 if (!glue)
  return 0;
 glue->p_wlan_data->suspend(glue->p_wlan_data->pm_param);
 for (i = 0 ; i < MAX_NR_RECVBUFF ; ++i) {
  usb_kill_urb(glue->ssv_rx_buf[i].rx_urb);
 }
 return 0;
}
static int ssv_usb_resume(struct usb_interface *interface)
{
 struct ssv6xxx_usb_glue *glue = usb_get_intfdata(interface);
 int i;
 int nr_recvbuff = (ssv_rx_nr_recvbuff > MAX_NR_RECVBUFF)?MAX_NR_RECVBUFF:((ssv_rx_nr_recvbuff < MIN_NR_RECVBUFF)?MIN_NR_RECVBUFF:ssv_rx_nr_recvbuff);
 dev_info(glue->dev, "%s(): resume.\n", __FUNCTION__);
 if (!glue)
  return 0;
 for (i = 0 ; i < nr_recvbuff ; ++i) {
  ssv6xxx_usb_recv_rx(glue, &(glue->ssv_rx_buf[i]));
 }
 glue->p_wlan_data->resume(glue->p_wlan_data->pm_param);
 return 0;
}
#endif
static struct usb_driver ssv_usb_driver = {
 .name = "SSV6XXX_USB",
 .probe = ssv_usb_probe,
 .disconnect = ssv_usb_disconnect,
#ifdef CONFIG_PM
 .suspend = ssv_usb_suspend,
 .resume = ssv_usb_resume,
#endif
 .id_table = ssv_usb_table,
 .supports_autosuspend = 1,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
    .disable_hub_initiated_lpm = 0,
#endif
};
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
int ssv6xxx_usb_init(void)
#else
static int __init ssv6xxx_usb_init(void)
#endif
{
 printk(KERN_INFO "ssv6xxx_usb_init\n");
 return usb_register(&ssv_usb_driver);
}
static int ssv_usb_do_device_exit(struct device *d, void *arg)
{
 struct usb_interface *intf = to_usb_interface(d);
 struct ssv6xxx_usb_glue *glue = usb_get_intfdata(intf);
 u32 regval;
 int ret;
 if (glue != NULL) {
  printk(KERN_INFO "ssv_usb_do_device_exit: JUMP to ROM\n");
  ret = __ssv6xxx_usb_read_reg(glue, 0xc000001c, &regval);
  if (__ssv6xxx_usb_write_reg(glue, 0xc000001c, (regval & 0xfeffffff)));
  ret = __ssv6xxx_usb_read_reg(glue, 0xc00000ec, &regval);
  if (__ssv6xxx_usb_write_reg(glue, 0xc00000ec, (regval & 0xffffefff)));
  ret = __ssv6xxx_usb_read_reg(glue, 0xc00000e8, &regval);
  if (__ssv6xxx_usb_write_reg(glue, 0xc00000e8, (regval | 0x00000004)));
  ret = __ssv6xxx_usb_read_reg(glue, 0xc000001c, &regval);
  if (__ssv6xxx_usb_write_reg(glue, 0xc000001c, (regval | 0x01000000)));
 }
 msleep(50);
 return 0;
}
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
void ssv6xxx_usb_exit(void)
#else
static void __exit ssv6xxx_usb_exit(void)
#endif
{
 if (driver_for_each_device(&ssv_usb_driver.drvwrap.driver, NULL,
  NULL, ssv_usb_do_device_exit)){};
    printk(KERN_INFO "ssv6xxx_usb_exit\n");
 usb_deregister(&ssv_usb_driver);
}
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
EXPORT_SYMBOL(ssv6xxx_usb_init);
EXPORT_SYMBOL(ssv6xxx_usb_exit);
#else
module_init(ssv6xxx_usb_init);
module_exit(ssv6xxx_usb_exit);
#endif
MODULE_LICENSE("GPL");
