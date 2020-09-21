#ifndef _GPXE_IBFT_H
#define _GPXE_IBFT_H

/*
 * Copyright Fen Systems Ltd. 2007.  Portions of this code are derived
 * from IBM Corporation Sample Programs.  Copyright IBM Corporation
 * 2004, 2007.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

FILE_LICENCE ( BSD2 );

/** @file
 *
 * iSCSI boot firmware table
 *
 * The information in this file is derived from the document "iSCSI
 * Boot Firmware Table (iBFT)" as published by IBM at
 *
 * ftp://ftp.software.ibm.com/systems/support/system_x_pdf/ibm_iscsi_boot_firmware_table_v1.02.pdf
 *
 */

#include <stdint.h>
#include <gpxe/acpi.h>
#include <gpxe/in.h>

/** iSCSI Boot Firmware Table signature */
#define IBFT_SIG "iBFT"

/** An offset from the start of the iBFT */
typedef uint16_t ibft_off_t;

/** Length of a string within the iBFT (excluding terminating NUL) */
typedef uint16_t ibft_size_t;

/** A string within the iBFT */
struct ibft_string {
	/** Length of string */
	ibft_size_t length;
	/** Offset to string */
	ibft_off_t offset;
} __attribute__ (( packed ));

/** An IP address within the iBFT */
struct ibft_ipaddr {
	/** Reserved; must be zero */
	uint16_t zeroes[5];
	/** Must be 0xffff if IPv4 address is present, otherwise zero */
	uint16_t ones;
	/** The IPv4 address, or zero if not present */
	struct in_addr in;
} __attribute__ (( packed ));

/**
 * iBFT structure header
 *
 * This structure is common to several sections within the iBFT.
 */
struct ibft_header {
	/** Structure ID
	 *
	 * This is an IBFT_STRUCTURE_ID_XXX constant
	 */
	uint8_t structure_id;
	/** Version (always 1) */
	uint8_t version;
	/** Length, including this header */
	uint16_t length;
	/** Index 
	 *
	 * This is the number of the NIC or Target, when applicable.
	 */
	uint8_t index;
	/** Flags */
	uint8_t flags;
} __attribute__ (( packed ));

/**
 * iBFT Control structure
 *
 */
struct ibft_control {
	/** Common header */
	struct ibft_header header;
	/** Extensions */
	uint16_t extensions;
	/** Offset to Initiator structure */
	ibft_off_t initiator;
	/** Offset to NIC structure for NIC 0 */
	ibft_off_t nic_0;
	/** Offset to Target structure for target 0 */
	ibft_off_t target_0;
	/** Offset to NIC structure for NIC 1 */
	ibft_off_t nic_1;
	/** Offset to Target structure for target 1 */
	ibft_off_t target_1;
} __attribute__ (( packed ));

/** Structure ID for Control section */
#define IBFT_STRUCTURE_ID_CONTROL 0x01

/** Attempt login only to specified target
 *
 * If this flag is not set, all targets will be logged in to.
 */
#define IBFT_FL_CONTROL_SINGLE_LOGIN_ONLY 0x01

/**
 * iBFT Initiator structure
 *
 */
struct ibft_initiator {
	/** Common header */
	struct ibft_header header;
	/** iSNS server */
	struct ibft_ipaddr isns_server;
	/** SLP server */
	struct ibft_ipaddr slp_server;
	/** Primary and secondary Radius servers */
	struct ibft_ipaddr radius[2];
	/** Initiator name */
	struct ibft_string initiator_name;
} __attribute__ (( packed ));

/** Structure ID for Initiator section */
#define IBFT_STRUCTURE_ID_INITIATOR 0x02

/** Initiator block valid */
#define IBFT_FL_INITIATOR_BLOCK_VALID 0x01

/** Initiator firmware boot selected */
#define IBFT_FL_INITIATOR_FIRMWARE_BOOT_SELECTED 0x02

/**
 * iBFT NIC structure
 *
 */
struct ibft_nic {
	/** Common header */
	struct ibft_header header;
	/** IP address */
	struct ibft_ipaddr ip_address;
	/** Subnet mask
	 *
	 * This is the length of the subnet mask in bits (e.g. /24).
	 */
	uint8_t subnet_mask_prefix;
	/** Origin */
	uint8_t origin;
	/** Default gateway */
	struct ibft_ipaddr gateway;
	/** Primary and secondary DNS servers */
	struct ibft_ipaddr dns[2];
	/** DHCP server */
	struct ibft_ipaddr dhcp;
	/** VLAN tag */
	uint16_t vlan;
	/** MAC address */
	uint8_t mac_address[6];
	/** PCI bus:dev:fn */
	uint16_t pci_bus_dev_func;
	/** Hostname */
	struct ibft_string hostname;
} __attribute__ (( packed ));

/** Structure ID for NIC section */
#define IBFT_STRUCTURE_ID_NIC 0x03

/** NIC block valid */
#define IBFT_FL_NIC_BLOCK_VALID 0x01

/** NIC firmware boot selected */
#define IBFT_FL_NIC_FIRMWARE_BOOT_SELECTED 0x02

/** NIC global / link local */
#define IBFT_FL_NIC_GLOBAL 0x04

/**
 * iBFT Target structure
 *
 */
struct ibft_target {
	/** Common header */
	struct ibft_header header;
	/** IP address */
	struct ibft_ipaddr ip_address;
	/** TCP port */
	uint16_t socket;
	/** Boot LUN */
	uint64_t boot_lun;
	/** CHAP type
	 *
	 * This is an IBFT_CHAP_XXX constant.
	 */
	uint8_t chap_type;
	/** NIC association */
	uint8_t nic_association;
	/** Target name */
	struct ibft_string target_name;
	/** CHAP name */
	struct ibft_string chap_name;
	/** CHAP secret */
	struct ibft_string chap_secret;
	/** Reverse CHAP name */
	struct ibft_string reverse_chap_name;
	/** Reverse CHAP secret */
	struct ibft_string reverse_chap_secret;
} __attribute__ (( packed ));

/** Structure ID for Target section */
#define IBFT_STRUCTURE_ID_TARGET 0x04

/** Target block valid */
#define IBFT_FL_TARGET_BLOCK_VALID 0x01

/** Target firmware boot selected */
#define IBFT_FL_TARGET_FIRMWARE_BOOT_SELECTED 0x02

/** Target use Radius CHAP */
#define IBFT_FL_TARGET_USE_CHAP 0x04

/** Target use Radius rCHAP */
#define IBFT_FL_TARGET_USE_RCHAP 0x08

/* Values for chap_type */
#define IBFT_CHAP_NONE		0	/**< No CHAP authentication */
#define IBFT_CHAP_ONE_WAY	1	/**< One-way CHAP */
#define IBFT_CHAP_MUTUAL	2	/**< Mutual CHAP */

/**
 * iSCSI Boot Firmware Table (iBFT)
 */
struct ibft_table {
	/** ACPI header */
	struct acpi_description_header acpi;
	/** Reserved */
	uint8_t reserved[12];
	/** Control structure */
	struct ibft_control control;
} __attribute__ (( packed ));

/**
 * iSCSI string block descriptor
 *
 * This is an internal structure that we use to keep track of the
 * allocation of string data.
 */
struct ibft_string_block {
	/** The iBFT containing these strings */
	struct ibft_table *table;
	/** Offset of first free byte within iBFT */
	unsigned int offset;
};

/** Amount of space reserved for strings in a gPXE iBFT */
#define IBFT_STRINGS_SIZE 384

/**
 * An iBFT created by gPXE
 *
 */
struct gpxe_ibft {
	/** The fixed section */
	struct ibft_table table;
	/** The Initiator section */
	struct ibft_initiator initiator __attribute__ (( aligned ( 16 ) ));
	/** The NIC section */
	struct ibft_nic nic __attribute__ (( aligned ( 16 ) ));
	/** The Target section */
	struct ibft_target target __attribute__ (( aligned ( 16 ) ));
	/** Strings block */
	char strings[IBFT_STRINGS_SIZE];
} __attribute__ (( packed, aligned ( 16 ) ));

struct net_device;
struct iscsi_session;

extern int ibft_fill_data ( struct net_device *netdev,
			    struct iscsi_session *iscsi );

#endif /* _GPXE_IBFT_H */
