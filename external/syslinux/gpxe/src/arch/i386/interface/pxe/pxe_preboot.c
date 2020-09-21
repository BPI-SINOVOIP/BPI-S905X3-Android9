/** @file
 *
 * PXE Preboot API
 *
 */

/* PXE API interface for Etherboot.
 *
 * Copyright (C) 2004 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <gpxe/uaccess.h>
#include <gpxe/dhcp.h>
#include <gpxe/fakedhcp.h>
#include <gpxe/device.h>
#include <gpxe/netdevice.h>
#include <gpxe/isapnp.h>
#include <gpxe/init.h>
#include <gpxe/if_ether.h>
#include <basemem_packet.h>
#include <biosint.h>
#include "pxe.h"
#include "pxe_call.h"

/* Avoid dragging in isapnp.o unnecessarily */
uint16_t isapnp_read_port;

/** Zero-based versions of PXENV_GET_CACHED_INFO::PacketType */
enum pxe_cached_info_indices {
	CACHED_INFO_DHCPDISCOVER = ( PXENV_PACKET_TYPE_DHCP_DISCOVER - 1 ),
	CACHED_INFO_DHCPACK = ( PXENV_PACKET_TYPE_DHCP_ACK - 1 ),
	CACHED_INFO_BINL = ( PXENV_PACKET_TYPE_CACHED_REPLY - 1 ),
	NUM_CACHED_INFOS
};

/** A cached DHCP packet */
union pxe_cached_info {
	struct dhcphdr dhcphdr;
	/* This buffer must be *exactly* the size of a BOOTPLAYER_t
	 * structure, otherwise WinPE will die horribly.  It takes the
	 * size of *our* buffer and feeds it in to us as the size of
	 * one of *its* buffers.  If our buffer is larger than it
	 * expects, we therefore end up overwriting part of its data
	 * segment, since it tells us to do so.  (D'oh!)
	 *
	 * Note that a BOOTPLAYER_t is not necessarily large enough to
	 * hold a DHCP packet; this is a flaw in the PXE spec.
	 */
	BOOTPLAYER_t packet;
} __attribute__ (( packed ));

/** A PXE DHCP packet creator */
struct pxe_dhcp_packet_creator {
	/** Create DHCP packet
	 *
	 * @v netdev		Network device
	 * @v data		Buffer for DHCP packet
	 * @v max_len		Size of DHCP packet buffer
	 * @ret rc		Return status code
	 */
	int ( * create ) ( struct net_device *netdev, void *data,
			   size_t max_len );
};

/** PXE DHCP packet creators */
static struct pxe_dhcp_packet_creator pxe_dhcp_packet_creators[] = {
	[CACHED_INFO_DHCPDISCOVER] = { create_fakedhcpdiscover },
	[CACHED_INFO_DHCPACK] = { create_fakedhcpack },
	[CACHED_INFO_BINL] = { create_fakepxebsack },
};

/* The case in which the caller doesn't supply a buffer is really
 * awkward to support given that we have multiple sources of options,
 * and that we don't actually store the DHCP packets.  (We may not
 * even have performed DHCP; we may have obtained all configuration
 * from non-volatile stored options or from the command line.)
 *
 * Some NBPs rely on the buffers we provide being persistent, so we
 * can't just use the temporary packet buffer.  4.5kB of base memory
 * always wasted just because some clients are too lazy to provide
 * their own buffers...
 */
static union pxe_cached_info __bss16_array ( cached_info, [NUM_CACHED_INFOS] );
#define cached_info __use_data16 ( cached_info )

/**
 * Set PXE cached TFTP filename
 *
 * @v filename		TFTP filename
 *
 * This is a bug-for-bug compatibility hack needed in order to work
 * with Microsoft Remote Installation Services (RIS).  The filename
 * used in a call to PXENV_RESTART_TFTP or PXENV_TFTP_READ_FILE must
 * be returned as the DHCP filename in subsequent calls to
 * PXENV_GET_CACHED_INFO.
 */
void pxe_set_cached_filename ( const unsigned char *filename ) {
	memcpy ( cached_info[CACHED_INFO_DHCPACK].dhcphdr.file, filename,
		 sizeof ( cached_info[CACHED_INFO_DHCPACK].dhcphdr.file ) );
	memcpy ( cached_info[CACHED_INFO_BINL].dhcphdr.file, filename,
		 sizeof ( cached_info[CACHED_INFO_BINL].dhcphdr.file ) );
}

/**
 * UNLOAD BASE CODE STACK
 *
 * @v None				-
 * @ret ...
 *
 */
PXENV_EXIT_t pxenv_unload_stack ( struct s_PXENV_UNLOAD_STACK *unload_stack ) {
	DBG ( "PXENV_UNLOAD_STACK" );

	unload_stack->Status = PXENV_STATUS_SUCCESS;
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_GET_CACHED_INFO
 *
 * Status: working
 */
PXENV_EXIT_t pxenv_get_cached_info ( struct s_PXENV_GET_CACHED_INFO
				     *get_cached_info ) {
	struct pxe_dhcp_packet_creator *creator;
	union pxe_cached_info *info;
	unsigned int idx;
	size_t len;
	userptr_t buffer;
	int rc;

	DBG ( "PXENV_GET_CACHED_INFO %d", get_cached_info->PacketType );

	DBG ( " to %04x:%04x+%x", get_cached_info->Buffer.segment,
	      get_cached_info->Buffer.offset, get_cached_info->BufferSize );

	/* Sanity check */
        idx = ( get_cached_info->PacketType - 1 );
	if ( idx >= NUM_CACHED_INFOS ) {
		DBG ( " bad PacketType" );
		goto err;
	}
	info = &cached_info[idx];

	/* Construct cached version of packet, if not already constructed. */
	if ( ! info->dhcphdr.op ) {
		/* Construct DHCP packet */
		creator = &pxe_dhcp_packet_creators[idx];
		if ( ( rc = creator->create ( pxe_netdev, info,
					      sizeof ( *info ) ) ) != 0 ) {
			DBG ( " failed to build packet" );
			goto err;
		}
	}

	len = get_cached_info->BufferSize;
	if ( len == 0 ) {
		/* Point client at our cached buffer.
		 *
		 * To add to the fun, Intel decided at some point in
		 * the evolution of the PXE specification to add the
		 * BufferLimit field, which we are meant to fill in
		 * with the length of our packet buffer, so that the
		 * caller can safely modify the boot server reply
		 * packet stored therein.  However, this field was not
		 * present in earlier versions of the PXE spec, and
		 * there is at least one PXE NBP (Altiris) which
		 * allocates only exactly enough space for this
		 * earlier, shorter version of the structure.  If we
		 * actually fill in the BufferLimit field, we
		 * therefore risk trashing random areas of the
		 * caller's memory.  If we *don't* fill it in, then
		 * the caller is at liberty to assume that whatever
		 * random value happened to be in that location
		 * represents the length of the buffer we've just
		 * passed back to it.
		 *
		 * Since older PXE stacks won't fill this field in
		 * anyway, it's probably safe to assume that no
		 * callers actually rely on it, so we choose to not
		 * fill it in.
		 */
		get_cached_info->Buffer.segment = rm_ds;
		get_cached_info->Buffer.offset = __from_data16 ( info );
		get_cached_info->BufferSize = sizeof ( *info );
		DBG ( " returning %04x:%04x+%04x['%x']",
		      get_cached_info->Buffer.segment,
		      get_cached_info->Buffer.offset,
		      get_cached_info->BufferSize,
		      get_cached_info->BufferLimit );
	} else {
		/* Copy packet to client buffer */
		if ( len > sizeof ( *info ) )
			len = sizeof ( *info );
		if ( len < sizeof ( *info ) )
			DBG ( " buffer may be too short" );
		buffer = real_to_user ( get_cached_info->Buffer.segment,
					get_cached_info->Buffer.offset );
		copy_to_user ( buffer, 0, info, len );
		get_cached_info->BufferSize = len;
	}

	get_cached_info->Status = PXENV_STATUS_SUCCESS;
	return PXENV_EXIT_SUCCESS;

 err:
	get_cached_info->Status = PXENV_STATUS_OUT_OF_RESOURCES;
	return PXENV_EXIT_FAILURE;
}

/* PXENV_RESTART_TFTP
 *
 * Status: working
 */
PXENV_EXIT_t pxenv_restart_tftp ( struct s_PXENV_TFTP_READ_FILE
				  *restart_tftp ) {
	PXENV_EXIT_t tftp_exit;

	DBG ( "PXENV_RESTART_TFTP " );

	/* Intel bug-for-bug hack */
	pxe_set_cached_filename ( restart_tftp->FileName );

	/* Words cannot describe the complete mismatch between the PXE
	 * specification and any possible version of reality...
	 */
	restart_tftp->Buffer = PXE_LOAD_PHYS; /* Fixed by spec, apparently */
	restart_tftp->BufferSize = ( 0xa0000 - PXE_LOAD_PHYS ); /* Near enough */
	tftp_exit = pxenv_tftp_read_file ( restart_tftp );
	if ( tftp_exit != PXENV_EXIT_SUCCESS )
		return tftp_exit;

	/* Fire up the new NBP */
	restart_tftp->Status = pxe_start_nbp();

	/* Not sure what "SUCCESS" actually means, since we can only
	 * return if the new NBP failed to boot...
	 */
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_START_UNDI
 *
 * Status: working
 */
PXENV_EXIT_t pxenv_start_undi ( struct s_PXENV_START_UNDI *start_undi ) {
	unsigned int bus_type;
	unsigned int location;
	struct net_device *netdev;

	DBG ( "PXENV_START_UNDI %04x:%04x:%04x",
	      start_undi->AX, start_undi->BX, start_undi->DX );

	/* Determine bus type and location.  Use a heuristic to decide
	 * whether we are PCI or ISAPnP
	 */
	if ( ( start_undi->DX >= ISAPNP_READ_PORT_MIN ) &&
	     ( start_undi->DX <= ISAPNP_READ_PORT_MAX ) &&
	     ( start_undi->BX >= ISAPNP_CSN_MIN ) &&
	     ( start_undi->BX <= ISAPNP_CSN_MAX ) ) {
		bus_type = BUS_TYPE_ISAPNP;
		location = start_undi->BX;
		/* Record ISAPnP read port for use by isapnp.c */
		isapnp_read_port = start_undi->DX;
	} else {
		bus_type = BUS_TYPE_PCI;
		location = start_undi->AX;
	}

	/* Probe for devices, etc. */
	startup();

	/* Look for a matching net device */
	netdev = find_netdev_by_location ( bus_type, location );
	if ( ! netdev ) {
		DBG ( " no net device found" );
		start_undi->Status = PXENV_STATUS_UNDI_CANNOT_INITIALIZE_NIC;
		return PXENV_EXIT_FAILURE;
	}
	DBG ( " using netdev %s", netdev->name );

	/* Activate PXE */
	pxe_activate ( netdev );

	start_undi->Status = PXENV_STATUS_SUCCESS;
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_STOP_UNDI
 *
 * Status: working
 */
PXENV_EXIT_t pxenv_stop_undi ( struct s_PXENV_STOP_UNDI *stop_undi ) {
	DBG ( "PXENV_STOP_UNDI" );

	/* Deactivate PXE */
	pxe_deactivate();

	/* Prepare for unload */
	shutdown ( SHUTDOWN_BOOT );

	/* Check to see if we still have any hooked interrupts */
	if ( hooked_bios_interrupts != 0 ) {
		DBG ( "PXENV_STOP_UNDI failed: %d interrupts still hooked\n",
		      hooked_bios_interrupts );
		stop_undi->Status = PXENV_STATUS_KEEP_UNDI;
		return PXENV_EXIT_FAILURE;
	}

	stop_undi->Status = PXENV_STATUS_SUCCESS;
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_START_BASE
 *
 * Status: won't implement (requires major structural changes)
 */
PXENV_EXIT_t pxenv_start_base ( struct s_PXENV_START_BASE *start_base ) {
	DBG ( "PXENV_START_BASE" );

	start_base->Status = PXENV_STATUS_UNSUPPORTED;
	return PXENV_EXIT_FAILURE;
}

/* PXENV_STOP_BASE
 *
 * Status: working
 */
PXENV_EXIT_t pxenv_stop_base ( struct s_PXENV_STOP_BASE *stop_base ) {
	DBG ( "PXENV_STOP_BASE" );

	/* The only time we will be called is when the NBP is trying
	 * to shut down the PXE stack.  There's nothing we need to do
	 * in this call.
	 */

	stop_base->Status = PXENV_STATUS_SUCCESS;
	return PXENV_EXIT_SUCCESS;
}
