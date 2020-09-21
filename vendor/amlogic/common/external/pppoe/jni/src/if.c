/***********************************************************************
*
* if.c
*
* Implementation of user-space PPPoE redirector for Linux.
*
* Functions for opening a raw socket and reading/writing raw Ethernet frames.
*
* Copyright (C) 2000 by Roaring Penguin Software Inc.
*
* This program may be distributed according to the terms of the GNU
* General Public License, version 2 or (at your option) any later version.
*
* LIC: GPL
*
***********************************************************************/

static char const RCSID[] =
"$Id$";

#include "pppoe.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_NETPACKET_PACKET_H
#include <netpacket/packet.h>
#elif defined(HAVE_LINUX_IF_PACKET_H)
#include <linux/if_packet.h>
#endif

#ifdef HAVE_NET_ETHERNET_H
//#include <net/ethernet.h>
#endif

#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYSLOG_H
#include <android/log.h>
#include <syslog.h>
#define syslog(prio, fmt...) \
    __android_log_print(prio, "PPPOE", fmt)
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#ifdef USE_DLPI

#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/dlpi.h>
#include <sys/bufmod.h>
#include <stdio.h>
#include <signal.h>
#include <stropts.h>

/* function declarations */

static void dlpromisconreq( int fd, u_long  level);
void dlinforeq(int fd);
void dlunitdatareq(int fd, u_char *addrp, int addrlen, u_long minpri, u_long maxpri, u_char *datap, int datalen);
void dlinfoack(int fd, char *bufp);
void dlbindreq(int fd, u_long sap, u_long max_conind, u_long service_mode, u_long conn_mgmt, u_long xidtest);
void dlattachreq(int fd, u_long ppa);
void dlokack(int fd, char *bufp);
void dlbindack(int fd, char *bufp);
int strioctl(int fd, int cmd, int timout, int len, char *dp);
void strgetmsg(int fd, struct strbuf *ctlp, struct strbuf *datap, int *flagsp, char *caller);
void sigalrm(int sig);
void expecting(int prim, union DL_primitives *dlp);
static char *dlprim(u_long prim);

/* #define DL_DEBUG */

static	int     dl_abssaplen;
static	int     dl_saplen;
static	int	dl_addrlen;

#endif

#ifdef USE_BPF
#include <net/bpf.h>
#include <fcntl.h>

static unsigned char *bpfBuffer;	/* Packet filter buffer */
static int bpfLength = 0;		/* Packet filter buffer length */
       int bpfSize = 0;		        /* Number of unread bytes in buffer */
static int bpfOffset = 0;		/* Current offset in bpfBuffer */
#endif

/* Initialize frame types to RFC 2516 values.  Some broken peers apparently
   use different frame types... sigh... */

UINT16_t Eth_PPPOE_Discovery = ETH_PPPOE_DISCOVERY;
UINT16_t Eth_PPPOE_Session   = ETH_PPPOE_SESSION;

/**********************************************************************
*%FUNCTION: etherType
*%ARGUMENTS:
* packet -- a received PPPoE packet
*%RETURNS:
* ethernet packet type (see /usr/include/net/ethertypes.h)
*%DESCRIPTION:
* Checks the ethernet packet header to determine its type.
* We should only be receveing DISCOVERY and SESSION types if the BPF
* is set up correctly.  Logs an error if an unexpected type is received.
* Note that the ethernet type names come from "pppoe.h" and the packet
* packet structure names use the LINUX dialect to maintain consistency
* with the rest of this file.  See the BSD section of "pppoe.h" for
* translations of the data structure names.
***********************************************************************/
UINT16_t
etherType(PPPoEPacket *packet)
{
    UINT16_t type = (UINT16_t) ntohs(packet->ethHdr.h_proto);
    if (type != Eth_PPPOE_Discovery && type != Eth_PPPOE_Session) {
	syslog(LOG_ERR, "Invalid ether type 0x%x", type);
    }
    return type;
}


#ifdef USE_LINUX_PACKET
/**********************************************************************
*%FUNCTION: openInterface
*%ARGUMENTS:
* ifname -- name of interface
* type -- Ethernet frame type
* hwaddr -- if non-NULL, set to the hardware address
*%RETURNS:
* A raw socket for talking to the Ethernet card.  Exits on error.
*%DESCRIPTION:
* Opens a raw Ethernet socket
***********************************************************************/
int
openInterface(char const *ifname, UINT16_t type, unsigned char *hwaddr)
{
    int optval=1;
    int fd;
    struct ifreq ifr;
    int domain, stype;

    struct sockaddr_ll sa;

    memset(&sa, 0, sizeof(sa));

    domain = PF_PACKET;
    stype = SOCK_RAW;

    if ((fd = socket(domain, stype, htons(type))) < 0) {
	/* Give a more helpful message for the common error case */
	if (errno == EPERM) {
	    rp_fatal("Cannot create raw socket -- pppoe must be run as root.");
	}
	fatalSys("socket");
    }

    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0) {
	fatalSys("setsockopt");
    }

    /* Fill in hardware address */
    if (hwaddr) {
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
	    fatalSys("ioctl(SIOCGIFHWADDR)");
	}
	memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	if (NOT_UNICAST(hwaddr)) {
	    char buffer[256];
	    sprintf(buffer,
		    "Interface %.16s has broadcast/multicast MAC address??",
		    ifname);
	    rp_fatal(buffer);
	}
    }

    /* Sanity check on MTU */
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(fd, SIOCGIFMTU, &ifr) < 0) {
	fatalSys("ioctl(SIOCGIFMTU)");
    }
    if (ifr.ifr_mtu < ETH_DATA_LEN) {
	char buffer[256];
	sprintf(buffer, "Interface %.16s has MTU of %d -- should be %d.  You may have serious connection problems.",
		ifname, ifr.ifr_mtu, ETH_DATA_LEN);
	printErr(buffer);
    }

    /* Get interface index */
    sa.sll_family = AF_PACKET;
    sa.sll_protocol = htons(type);

    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
	fatalSys("ioctl(SIOCFIGINDEX): Could not get interface index");
    }
    sa.sll_ifindex = ifr.ifr_ifindex;

    /* We're only interested in packets on specified interface */
    if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
	fatalSys("bind");
    }

    return fd;
}

#endif /* USE_LINUX */

/***********************************************************************
*%FUNCTION: sendPacket
*%ARGUMENTS:
* sock -- socket to send to
* pkt -- the packet to transmit
* size -- size of packet (in bytes)
*%RETURNS:
* 0 on success; -1 on failure
*%DESCRIPTION:
* Transmits a packet
***********************************************************************/
int
sendPacket(PPPoEConnection *conn, int sock, PPPoEPacket *pkt, int size)
{
    if (send(sock, pkt, size, 0) < 0 && (errno != ENOBUFS)) {
	sysErr("send (sendPacket)");
	return -1;
    }
    return 0;
}

#ifdef USE_BPF
/***********************************************************************
*%FUNCTION: clearPacketHeader
*%ARGUMENTS:
* pkt -- packet that needs its head clearing
*%RETURNS:
* nothing
*%DESCRIPTION:
* Clears a PPPoE packet header after a truncated packet has been
* received.  Insures that the packet will fail any integrity tests
* and will be discarded by upper level routines.  Also resets the
* bpfSize and bpfOffset variables to force a new read on the next
* call to receivePacket().
***********************************************************************/
void
clearPacketHeader(PPPoEPacket *pkt)
{
    bpfSize = bpfOffset = 0;
    memset(pkt, 0, HDR_SIZE);
}
#endif

/***********************************************************************
*%FUNCTION: receivePacket
*%ARGUMENTS:
* sock -- socket to read from
* pkt -- place to store the received packet
* size -- set to size of packet in bytes
*%RETURNS:
* >= 0 if all OK; < 0 if error
*%DESCRIPTION:
* Receives a packet
***********************************************************************/
int
receivePacket(int sock, PPPoEPacket *pkt, int *size)
{
#ifdef USE_BPF
    struct bpf_hdr hdr;
    int seglen, copylen;

    if (bpfSize <= 0) {
	bpfOffset = 0;
	if ((bpfSize = read(sock, bpfBuffer, bpfLength)) < 0) {
	    sysErr("read (receivePacket)");
	    return -1;
	}
    }
    if (bpfSize < sizeof(hdr)) {
	syslog(LOG_ERR, "Truncated bpf packet header: len=%d", bpfSize);
	clearPacketHeader(pkt);		/* resets bpfSize and bpfOffset */
	return 0;
    }
    memcpy(&hdr, bpfBuffer + bpfOffset, sizeof(hdr));
    if (hdr.bh_caplen != hdr.bh_datalen) {
	syslog(LOG_ERR, "Truncated bpf packet: caplen=%d, datalen=%d",
	       hdr.bh_caplen, hdr.bh_datalen);
	clearPacketHeader(pkt);		/* resets bpfSize and bpfOffset */
	return 0;
    }
    seglen = hdr.bh_hdrlen + hdr.bh_caplen;
    if (seglen > bpfSize) {
	syslog(LOG_ERR, "Truncated bpf packet: seglen=%d, bpfSize=%d",
	       seglen, bpfSize);
	clearPacketHeader(pkt);		/* resets bpfSize and bpfOffset */
	return 0;
    }
    seglen = BPF_WORDALIGN(seglen);
    *size = copylen = ((hdr.bh_caplen < sizeof(PPPoEPacket)) ?
			hdr.bh_caplen : sizeof(PPPoEPacket));
    memcpy(pkt, bpfBuffer + bpfOffset + hdr.bh_hdrlen, copylen);
    if (seglen >= bpfSize) {
	bpfSize = bpfOffset = 0;
    } else {
	bpfSize -= seglen;
	bpfOffset += seglen;
    }
#else
#ifdef USE_DLPI
	struct strbuf data;
	int flags = 0;
	int retval;

	data.buf = (char *) pkt;
	data.maxlen = MAXDLBUF;
	data.len = 0;

	if ((retval = getmsg(sock, NULL, &data, &flags)) < 0) {
	    sysErr("read (receivePacket)");
	    return -1;
	}

	*size = data.len;

#else
    if ((*size = recv(sock, pkt, sizeof(PPPoEPacket), 0)) < 0) {
	sysErr("recv (receivePacket)");
	return -1;
    }
#endif
#endif
    return 0;
}

