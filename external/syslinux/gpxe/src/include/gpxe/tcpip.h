#ifndef _GPXE_TCPIP_H
#define _GPXE_TCPIP_H

/** @file
 *
 * Transport-network layer interface
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <gpxe/socket.h>
#include <gpxe/in.h>
#include <gpxe/tables.h>

struct io_buffer;
struct net_device;

/** Empty checksum value
 *
 * This is the TCP/IP checksum over a zero-length block of data.
 */
#define TCPIP_EMPTY_CSUM 0xffff

/**
 * TCP/IP socket address
 *
 * This contains the fields common to socket addresses for all TCP/IP
 * address families.
 */
struct sockaddr_tcpip {
	/** Socket address family (part of struct @c sockaddr) */
	sa_family_t st_family;
	/** TCP/IP port */
	uint16_t st_port;
	/** Padding
	 *
	 * This ensures that a struct @c sockaddr_tcpip is large
	 * enough to hold a socket address for any TCP/IP address
	 * family.
	 */
	char pad[ sizeof ( struct sockaddr ) -
		  ( sizeof ( sa_family_t ) + sizeof ( uint16_t ) ) ];
} __attribute__ (( may_alias ));

/** 
 * A transport-layer protocol of the TCP/IP stack (eg. UDP, TCP, etc)
 */
struct tcpip_protocol {
	/** Protocol name */
	const char *name;
       	/**
         * Process received packet
         *
         * @v iobuf		I/O buffer
	 * @v st_src		Partially-filled source address
	 * @v st_dest		Partially-filled destination address
	 * @v pshdr_csum	Pseudo-header checksum
	 * @ret rc		Return status code
         *
         * This method takes ownership of the I/O buffer.
         */
        int ( * rx ) ( struct io_buffer *iobuf, struct sockaddr_tcpip *st_src,
		       struct sockaddr_tcpip *st_dest, uint16_t pshdr_csum );
        /** 
	 * Transport-layer protocol number
	 *
	 * This is a constant of the type IP_XXX
         */
        uint8_t tcpip_proto;
};

/**
 * A network-layer protocol of the TCP/IP stack (eg. IPV4, IPv6, etc)
 */
struct tcpip_net_protocol {
	/** Protocol name */
	const char *name;
	/** Network address family */
	sa_family_t sa_family;
	/**
	 * Transmit packet
	 *
	 * @v iobuf		I/O buffer
	 * @v tcpip_protocol	Transport-layer protocol
	 * @v st_src		Source address, or NULL to use default
	 * @v st_dest		Destination address
	 * @v netdev		Network device (or NULL to route automatically)
	 * @v trans_csum	Transport-layer checksum to complete, or NULL
	 * @ret rc		Return status code
	 *
	 * This function takes ownership of the I/O buffer.
	 */
	int ( * tx ) ( struct io_buffer *iobuf,
		       struct tcpip_protocol *tcpip_protocol,
		       struct sockaddr_tcpip *st_src,
		       struct sockaddr_tcpip *st_dest,
		       struct net_device *netdev,
		       uint16_t *trans_csum );
};

/** TCP/IP transport-layer protocol table */
#define TCPIP_PROTOCOLS __table ( struct tcpip_protocol, "tcpip_protocols" )

/** Declare a TCP/IP transport-layer protocol */
#define __tcpip_protocol __table_entry ( TCPIP_PROTOCOLS, 01 )

/** TCP/IP network-layer protocol table */
#define TCPIP_NET_PROTOCOLS \
	__table ( struct tcpip_net_protocol, "tcpip_net_protocols" )

/** Declare a TCP/IP network-layer protocol */
#define __tcpip_net_protocol __table_entry ( TCPIP_NET_PROTOCOLS, 01 )

extern int tcpip_rx ( struct io_buffer *iobuf, uint8_t tcpip_proto,
		      struct sockaddr_tcpip *st_src,
		      struct sockaddr_tcpip *st_dest, uint16_t pshdr_csum );
extern int tcpip_tx ( struct io_buffer *iobuf, struct tcpip_protocol *tcpip,
		      struct sockaddr_tcpip *st_src,
		      struct sockaddr_tcpip *st_dest,
		      struct net_device *netdev,
		      uint16_t *trans_csum );
extern uint16_t tcpip_continue_chksum ( uint16_t partial,
					const void *data, size_t len );
extern uint16_t tcpip_chksum ( const void *data, size_t len );

#endif /* _GPXE_TCPIP_H */
