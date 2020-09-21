/*
 * Copyright (C) 2008 Michael Brown <mbrown@fensystems.co.uk>.
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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <gpxe/netdevice.h>
#include <gpxe/dhcp.h>
#include <gpxe/dhcpopts.h>
#include <gpxe/dhcppkt.h>

/** @file
 *
 * DHCP packets
 *
 */

/****************************************************************************
 *
 * DHCP packet raw interface
 *
 */

/**
 * Calculate used length of an IPv4 field within a DHCP packet
 *
 * @v data		Field data
 * @v len		Length of field
 * @ret used		Used length of field
 */
static size_t used_len_ipv4 ( const void *data, size_t len __unused ) {
	const struct in_addr *in = data;

	return ( in->s_addr ? sizeof ( *in ) : 0 );
}

/**
 * Calculate used length of a string field within a DHCP packet
 *
 * @v data		Field data
 * @v len		Length of field
 * @ret used		Used length of field
 */
static size_t used_len_string ( const void *data, size_t len ) {
	return strnlen ( data, len );
}

/** A dedicated field within a DHCP packet */
struct dhcp_packet_field {
	/** Settings tag number */
	unsigned int tag;
	/** Offset within DHCP packet */
	uint16_t offset;
	/** Length of field */
	uint16_t len;
	/** Calculate used length of field
	 *
	 * @v data	Field data
	 * @v len	Length of field
	 * @ret used	Used length of field
	 */
	size_t ( * used_len ) ( const void *data, size_t len );
};

/** Declare a dedicated field within a DHCP packet
 *
 * @v _tag		Settings tag number
 * @v _field		Field name
 * @v _used_len		Function to calculate used length of field
 */
#define DHCP_PACKET_FIELD( _tag, _field, _used_len ) {			\
		.tag = (_tag),						\
		.offset = offsetof ( struct dhcphdr, _field ),		\
		.len = sizeof ( ( ( struct dhcphdr * ) 0 )->_field ),	\
		.used_len = _used_len,					\
	}

/** Dedicated fields within a DHCP packet */
static struct dhcp_packet_field dhcp_packet_fields[] = {
	DHCP_PACKET_FIELD ( DHCP_EB_YIADDR, yiaddr, used_len_ipv4 ),
	DHCP_PACKET_FIELD ( DHCP_EB_SIADDR, siaddr, used_len_ipv4 ),
	DHCP_PACKET_FIELD ( DHCP_TFTP_SERVER_NAME, sname, used_len_string ),
	DHCP_PACKET_FIELD ( DHCP_BOOTFILE_NAME, file, used_len_string ),
};

/**
 * Get address of a DHCP packet field
 *
 * @v dhcphdr		DHCP packet header
 * @v field		DHCP packet field
 * @ret data		Packet field data
 */
static inline void * dhcp_packet_field ( struct dhcphdr *dhcphdr,
					 struct dhcp_packet_field *field ) {
	return ( ( ( void * ) dhcphdr ) + field->offset );
}

/**
 * Find DHCP packet field corresponding to settings tag number
 *
 * @v tag		Settings tag number
 * @ret field		DHCP packet field, or NULL
 */
static struct dhcp_packet_field *
find_dhcp_packet_field ( unsigned int tag ) {
	struct dhcp_packet_field *field;
	unsigned int i;

	for ( i = 0 ; i < ( sizeof ( dhcp_packet_fields ) /
			    sizeof ( dhcp_packet_fields[0] ) ) ; i++ ) {
		field = &dhcp_packet_fields[i];
		if ( field->tag == tag )
			return field;
	}
	return NULL;
}

/**
 * Store value of DHCP packet setting
 *
 * @v dhcppkt		DHCP packet
 * @v tag		Setting tag number
 * @v data		Setting data, or NULL to clear setting
 * @v len		Length of setting data
 * @ret rc		Return status code
 */
int dhcppkt_store ( struct dhcp_packet *dhcppkt, unsigned int tag,
		    const void *data, size_t len ) {
	struct dhcp_packet_field *field;
	void *field_data;
	int rc;

	/* If this is a special field, fill it in */
	if ( ( field = find_dhcp_packet_field ( tag ) ) != NULL ) {
		if ( len > field->len )
			return -ENOSPC;
		field_data = dhcp_packet_field ( dhcppkt->dhcphdr, field );
		memset ( field_data, 0, field->len );
		memcpy ( dhcp_packet_field ( dhcppkt->dhcphdr, field ),
			 data, len );
		/* Erase any equivalent option from the options block */
		dhcpopt_store ( &dhcppkt->options, tag, NULL, 0 );
		return 0;
	}

	/* Otherwise, use the generic options block */
	rc = dhcpopt_store ( &dhcppkt->options, tag, data, len );

	/* Update our used-length field */
	dhcppkt->len = ( offsetof ( struct dhcphdr, options ) +
			 dhcppkt->options.len );

	return rc;
}

/**
 * Fetch value of DHCP packet setting
 *
 * @v dhcppkt		DHCP packet
 * @v tag		Setting tag number
 * @v data		Buffer to fill with setting data
 * @v len		Length of buffer
 * @ret len		Length of setting data, or negative error
 */
int dhcppkt_fetch ( struct dhcp_packet *dhcppkt, unsigned int tag,
		    void *data, size_t len ) {
	struct dhcp_packet_field *field;
	void *field_data;
	size_t field_len = 0;
	
	/* Identify special field, if any */
	if ( ( field = find_dhcp_packet_field ( tag ) ) != NULL ) {
		field_data = dhcp_packet_field ( dhcppkt->dhcphdr, field );
		field_len = field->used_len ( field_data, field->len );
	}

	/* Return special field, if it exists and is populated */
	if ( field_len ) {
		if ( len > field_len )
			len = field_len;
		memcpy ( data, field_data, len );
		return field_len;
	}

	/* Otherwise, use the generic options block */
	return dhcpopt_fetch ( &dhcppkt->options, tag, data, len );
}

/****************************************************************************
 *
 * DHCP packet settings interface
 *
 */

/**
 * Store value of DHCP setting
 *
 * @v settings		Settings block
 * @v setting		Setting to store
 * @v data		Setting data, or NULL to clear setting
 * @v len		Length of setting data
 * @ret rc		Return status code
 */
static int dhcppkt_settings_store ( struct settings *settings,
				    struct setting *setting,
				    const void *data, size_t len ) {
	struct dhcp_packet *dhcppkt =
		container_of ( settings, struct dhcp_packet, settings );

	return dhcppkt_store ( dhcppkt, setting->tag, data, len );
}

/**
 * Fetch value of DHCP setting
 *
 * @v settings		Settings block, or NULL to search all blocks
 * @v setting		Setting to fetch
 * @v data		Buffer to fill with setting data
 * @v len		Length of buffer
 * @ret len		Length of setting data, or negative error
 */
static int dhcppkt_settings_fetch ( struct settings *settings,
				    struct setting *setting,
				    void *data, size_t len ) {
	struct dhcp_packet *dhcppkt =
		container_of ( settings, struct dhcp_packet, settings );

	return dhcppkt_fetch ( dhcppkt, setting->tag, data, len );
}

/** DHCP settings operations */
static struct settings_operations dhcppkt_settings_operations = {
	.store = dhcppkt_settings_store,
	.fetch = dhcppkt_settings_fetch,
};

/****************************************************************************
 *
 * Constructor
 *
 */

/**
 * Initialise DHCP packet
 *
 * @v dhcppkt		DHCP packet structure to fill in
 * @v data		DHCP packet raw data
 * @v max_len		Length of raw data buffer
 *
 * Initialise a DHCP packet structure from a data buffer containing a
 * DHCP packet.
 */
void dhcppkt_init ( struct dhcp_packet *dhcppkt, struct dhcphdr *data,
		    size_t len ) {
	dhcppkt->dhcphdr = data;
	dhcppkt->max_len = len;
	dhcpopt_init ( &dhcppkt->options, &dhcppkt->dhcphdr->options,
		       ( len - offsetof ( struct dhcphdr, options ) ) );
	dhcppkt->len = ( offsetof ( struct dhcphdr, options ) +
			 dhcppkt->options.len );
	settings_init ( &dhcppkt->settings,
			&dhcppkt_settings_operations, &dhcppkt->refcnt,
			DHCP_SETTINGS_NAME, 0 );
}
