/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Definitions from Linux bluetooth directory that are useful for
 * bluetotoh audio.
 * TODO(hychao): Remove this file when there is bluetooth user
 * space header provided.
 */

#include <unistd.h>

#define HCI_VIRTUAL     0
#define HCI_USB         1
#define HCI_PCCARD      2
#define HCI_UART        3
#define HCI_RS232       4
#define HCI_PCI         5
#define HCI_SDIO        6
#define HCI_BUS_MAX     7

#define BTPROTO_HCI 1
#define BTPROTO_SCO 2

#define SCO_OPTIONS   0x01
#define SOL_SCO 17

#define HCIGETDEVINFO   _IOR('H', 211, int)

typedef struct {
	uint8_t b[6];
} __attribute__ ((packed)) bdaddr_t;

struct hci_dev_stats {
	uint32_t err_rx;
	uint32_t err_tx;
	uint32_t cmd_tx;
	uint32_t evt_rx;
	uint32_t acl_tx;
	uint32_t acl_rx;
	uint32_t sco_tx;
	uint32_t sco_rx;
	uint32_t byte_rx;
	uint32_t byte_tx;
};

struct hci_dev_info {
	uint16_t dev_id;
	char  name[8];
	bdaddr_t bdaddr;
	uint32_t flags;
	uint8_t  type;
	uint8_t  features[8];
	uint32_t pkt_type;
	uint32_t link_policy;
	uint32_t link_mode;
	uint16_t acl_mtu;
	uint16_t acl_pkts;
	uint16_t sco_mtu;
	uint16_t sco_pkts;
	struct hci_dev_stats stat;
};

struct sco_options {
	uint16_t mtu;
};
