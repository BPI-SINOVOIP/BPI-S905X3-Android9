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

#ifndef _USB_DEF_H_
#define _USB_DEF_H_ 
#include <ssv_data_struct.h>
#define USB_DBG(fmt,...) pr_debug(fmt "\n", ##__VA_ARGS__)
#define FW_START_ADDR 0x00
#define FIRMWARE_DOWNLOAD 0xf0
#define SSV_EP_CMD 0x01
#define SSV_EP_RSP 0x02
#define SSV_EP_TX 0x03
#define SSV_EP_RX 0x04
#define SSV6200_CMD_WRITE_REG 0x01
#define SSV6200_CMD_READ_REG 0x02
struct ssv6xxx_read_reg_result {
 u32 value;
}__attribute__ ((packed));
struct ssv6xxx_read_reg {
 u32 addr;
 u32 value;
}__attribute__ ((packed));
struct ssv6xxx_write_reg {
 u32 addr;
 u32 value;
}__attribute__ ((packed));
union ssv6xxx_payload {
 struct ssv6xxx_read_reg rreg;
 struct ssv6xxx_read_reg_result rreg_res;
 struct ssv6xxx_write_reg wreg;
};
struct ssv6xxx_cmd_hdr {
 u8 plen;
 u8 cmd;
 u16 seq;
 union ssv6xxx_payload payload;
}__attribute__ ((packed));
struct ssv6xxx_cmd_endpoint {
 u8 address;
 u16 packet_size;
 void *buff;
};
struct ssv6xxx_tx_endpoint {
 u8 address;
 u16 packet_size;
 int tx_res;
};
struct ssv6xxx_rx_endpoint {
 u8 address;
 u16 packet_size;
};
#define MAX_NR_RECVBUFF (8)
#define MIN_NR_RECVBUFF (1)
struct ssv6xxx_rx_buf {
    struct ssv6xxx_list_node node;
    struct ssv6xxx_usb_glue *glue;
    struct urb *rx_urb;
    void *rx_buf;
    unsigned int rx_filled;
    int rx_res;
};
struct ssv6xxx_usb_work_struct {
    struct work_struct work;
    struct ssv6xxx_usb_glue *glue;
};
#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
int ssv6xxx_usb_init(void);
void ssv6xxx_usb_exit(void);
#endif
#endif
