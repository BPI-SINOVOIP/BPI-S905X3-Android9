/*
 * Copyright (c) 1991, 1992 Paul Kranenburg <pk@cs.few.eur.nl>
 * Copyright (c) 1993 Branko Lankester <branko@hacktic.nl>
 * Copyright (c) 1993, 1994, 1995, 1996 Rick Sladkey <jrs@world.std.com>
 * Copyright (c) 1996-2000 Wichert Akkerman <wichert@cistron.nl>
 * Copyright (c) 1999-2018 The strace developers.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "defs.h"
#include "print_fields.h"

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netinet/in.h>
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#ifdef HAVE_NETINET_UDP_H
# include <netinet/udp.h>
#endif
#ifdef HAVE_NETINET_SCTP_H
# include <netinet/sctp.h>
#endif
#include <arpa/inet.h>
#include <net/if.h>
#include <asm/types.h>
#ifdef HAVE_NETIPX_IPX_H
# include <netipx/ipx.h>
#else
# include <linux/ipx.h>
#endif

#if defined(HAVE_LINUX_IP_VS_H)
# include <linux/ip_vs.h>
#endif
#include "netlink.h"
#if defined(HAVE_LINUX_NETFILTER_ARP_ARP_TABLES_H)
# include <linux/netfilter_arp/arp_tables.h>
#endif
#if defined(HAVE_LINUX_NETFILTER_BRIDGE_EBTABLES_H)
# include <linux/netfilter_bridge/ebtables.h>
#endif
#if defined(HAVE_LINUX_NETFILTER_IPV4_IP_TABLES_H)
# include <linux/netfilter_ipv4/ip_tables.h>
#endif
#if defined(HAVE_LINUX_NETFILTER_IPV6_IP6_TABLES_H)
# include <linux/netfilter_ipv6/ip6_tables.h>
#endif
#include <linux/if_packet.h>
#include <linux/icmp.h>

#include "xlat/socktypes.h"
#include "xlat/sock_type_flags.h"
#ifndef SOCK_TYPE_MASK
# define SOCK_TYPE_MASK 0xf
#endif

#include "xlat/socketlayers.h"

#include "xlat/inet_protocols.h"

#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
# include <bluetooth/bluetooth.h>
# include "xlat/bt_protocols.h"
#endif

static void
decode_sockbuf(struct tcb *const tcp, const int fd, const kernel_ulong_t addr,
	       const kernel_ulong_t addrlen)
{

	switch (verbose(tcp) ? getfdproto(tcp, fd) : SOCK_PROTO_UNKNOWN) {
	case SOCK_PROTO_NETLINK:
		decode_netlink(tcp, fd, addr, addrlen);
		break;
	default:
		printstrn(tcp, addr, addrlen);
	}
}

/*
 * low bits of the socket type define real socket type,
 * other bits are socket type flags.
 */
static void
tprint_sock_type(unsigned int flags)
{
	const char *str = xlookup(socktypes, flags & SOCK_TYPE_MASK);

	if (str) {
		tprints(str);
		flags &= ~SOCK_TYPE_MASK;
		if (!flags)
			return;
		tprints("|");
	}
	printflags(sock_type_flags, flags, "SOCK_???");
}

SYS_FUNC(socket)
{
	printxval(addrfams, tcp->u_arg[0], "AF_???");
	tprints(", ");
	tprint_sock_type(tcp->u_arg[1]);
	tprints(", ");
	switch (tcp->u_arg[0]) {
	case AF_INET:
	case AF_INET6:
		printxval(inet_protocols, tcp->u_arg[2], "IPPROTO_???");
		break;

	case AF_NETLINK:
		printxval(netlink_protocols, tcp->u_arg[2], "NETLINK_???");
		break;

#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
	case AF_BLUETOOTH:
		printxval(bt_protocols, tcp->u_arg[2], "BTPROTO_???");
		break;
#endif

	default:
		tprintf("%" PRI_klu, tcp->u_arg[2]);
		break;
	}

	return RVAL_DECODED | RVAL_FD;
}

static bool
fetch_socklen(struct tcb *const tcp, int *const plen,
	      const kernel_ulong_t sockaddr, const kernel_ulong_t socklen)
{
	return verbose(tcp) && sockaddr && socklen
	       && umove(tcp, socklen, plen) == 0;
}

static int
decode_sockname(struct tcb *tcp)
{
	int ulen, rlen;

	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		if (fetch_socklen(tcp, &ulen, tcp->u_arg[1], tcp->u_arg[2])) {
			set_tcb_priv_ulong(tcp, ulen);
			return 0;
		} else {
			printaddr(tcp->u_arg[1]);
			tprints(", ");
			printaddr(tcp->u_arg[2]);
			return RVAL_DECODED;
		}
	}

	ulen = get_tcb_priv_ulong(tcp);

	if (syserror(tcp) || umove(tcp, tcp->u_arg[2], &rlen) < 0) {
		printaddr(tcp->u_arg[1]);
		tprintf(", [%d]", ulen);
	} else {
		decode_sockaddr(tcp, tcp->u_arg[1], ulen > rlen ? rlen : ulen);
		if (ulen != rlen)
			tprintf(", [%d->%d]", ulen, rlen);
		else
			tprintf(", [%d]", rlen);
	}

	return RVAL_DECODED;
}

SYS_FUNC(accept)
{
	return decode_sockname(tcp) | RVAL_FD;
}

SYS_FUNC(accept4)
{
	int rc = decode_sockname(tcp);

	if (rc & RVAL_DECODED) {
		tprints(", ");
		printflags(sock_type_flags, tcp->u_arg[3], "SOCK_???");
	}

	return rc | RVAL_FD;
}

SYS_FUNC(send)
{
	printfd(tcp, tcp->u_arg[0]);
	tprints(", ");
	decode_sockbuf(tcp, tcp->u_arg[0], tcp->u_arg[1], tcp->u_arg[2]);
	tprintf(", %" PRI_klu ", ", tcp->u_arg[2]);
	/* flags */
	printflags(msg_flags, tcp->u_arg[3], "MSG_???");

	return RVAL_DECODED;
}

SYS_FUNC(sendto)
{
	printfd(tcp, tcp->u_arg[0]);
	tprints(", ");
	decode_sockbuf(tcp, tcp->u_arg[0], tcp->u_arg[1], tcp->u_arg[2]);
	tprintf(", %" PRI_klu ", ", tcp->u_arg[2]);
	/* flags */
	printflags(msg_flags, tcp->u_arg[3], "MSG_???");
	/* to address */
	const int addrlen = tcp->u_arg[5];
	tprints(", ");
	decode_sockaddr(tcp, tcp->u_arg[4], addrlen);
	/* to length */
	tprintf(", %d", addrlen);

	return RVAL_DECODED;
}

SYS_FUNC(recv)
{
	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
	} else {
		if (syserror(tcp)) {
			printaddr(tcp->u_arg[1]);
		} else {
			decode_sockbuf(tcp, tcp->u_arg[0], tcp->u_arg[1],
				     tcp->u_rval);
		}

		tprintf(", %" PRI_klu ", ", tcp->u_arg[2]);
		printflags(msg_flags, tcp->u_arg[3], "MSG_???");
	}
	return 0;
}

SYS_FUNC(recvfrom)
{
	int ulen, rlen;

	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		if (fetch_socklen(tcp, &ulen, tcp->u_arg[4], tcp->u_arg[5])) {
			set_tcb_priv_ulong(tcp, ulen);
		}
	} else {
		/* buf */
		if (syserror(tcp)) {
			printaddr(tcp->u_arg[1]);
		} else {
			decode_sockbuf(tcp, tcp->u_arg[0], tcp->u_arg[1],
				     tcp->u_rval);
		}
		/* size */
		tprintf(", %" PRI_klu ", ", tcp->u_arg[2]);
		/* flags */
		printflags(msg_flags, tcp->u_arg[3], "MSG_???");
		tprints(", ");

		ulen = get_tcb_priv_ulong(tcp);

		if (!fetch_socklen(tcp, &rlen, tcp->u_arg[4], tcp->u_arg[5])) {
			/* from address */
			printaddr(tcp->u_arg[4]);
			tprints(", ");
			/* from length */
			printaddr(tcp->u_arg[5]);
			return 0;
		}
		if (syserror(tcp)) {
			/* from address */
			printaddr(tcp->u_arg[4]);
			/* from length */
			tprintf(", [%d]", ulen);
			return 0;
		}
		/* from address */
		decode_sockaddr(tcp, tcp->u_arg[4], ulen > rlen ? rlen : ulen);
		/* from length */
		if (ulen != rlen)
			tprintf(", [%d->%d]", ulen, rlen);
		else
			tprintf(", [%d]", rlen);
	}
	return 0;
}

SYS_FUNC(getsockname)
{
	return decode_sockname(tcp);
}

static void
printpair_fd(struct tcb *tcp, const int i0, const int i1)
{
	tprints("[");
	printfd(tcp, i0);
	tprints(", ");
	printfd(tcp, i1);
	tprints("]");
}

static void
decode_pair_fd(struct tcb *const tcp, const kernel_ulong_t addr)
{
	int pair[2];

	if (umove_or_printaddr(tcp, addr, &pair))
		return;

	printpair_fd(tcp, pair[0], pair[1]);
}

static int
do_pipe(struct tcb *tcp, int flags_arg)
{
	if (exiting(tcp)) {
		decode_pair_fd(tcp, tcp->u_arg[0]);
		if (flags_arg >= 0) {
			tprints(", ");
			printflags(open_mode_flags, tcp->u_arg[flags_arg], "O_???");
		}
	}
	return 0;
}

SYS_FUNC(pipe)
{
#if HAVE_ARCH_GETRVAL2
	if (exiting(tcp) && !syserror(tcp))
		printpair_fd(tcp, tcp->u_rval, getrval2(tcp));
	return 0;
#else
	return do_pipe(tcp, -1);
#endif
}

SYS_FUNC(pipe2)
{
	return do_pipe(tcp, 1);
}

SYS_FUNC(socketpair)
{
	if (entering(tcp)) {
		printxval(addrfams, tcp->u_arg[0], "AF_???");
		tprints(", ");
		tprint_sock_type(tcp->u_arg[1]);
		tprintf(", %" PRI_klu, tcp->u_arg[2]);
	} else {
		tprints(", ");
		decode_pair_fd(tcp, tcp->u_arg[3]);
	}
	return 0;
}

#include "xlat/sockoptions.h"
#include "xlat/sockipoptions.h"
#include "xlat/getsockipoptions.h"
#include "xlat/setsockipoptions.h"
#include "xlat/sockipv6options.h"
#include "xlat/getsockipv6options.h"
#include "xlat/setsockipv6options.h"
#include "xlat/sockipxoptions.h"
#include "xlat/socknetlinkoptions.h"
#include "xlat/sockpacketoptions.h"
#include "xlat/sockrawoptions.h"
#include "xlat/socksctpoptions.h"
#include "xlat/socktcpoptions.h"

static void
print_sockopt_fd_level_name(struct tcb *tcp, int fd, unsigned int level,
			    unsigned int name, bool is_getsockopt)
{
	printfd(tcp, fd);
	tprints(", ");
	printxval(socketlayers, level, "SOL_??");
	tprints(", ");

	switch (level) {
	case SOL_SOCKET:
		printxval(sockoptions, name, "SO_???");
		break;
	case SOL_IP:
		printxvals(name, "IP_???", sockipoptions,
			is_getsockopt ? getsockipoptions : setsockipoptions, NULL);
		break;
	case SOL_IPV6:
		printxvals(name, "IPV6_???", sockipv6options,
			is_getsockopt ? getsockipv6options : setsockipv6options, NULL);
		break;
	case SOL_IPX:
		printxval(sockipxoptions, name, "IPX_???");
		break;
	case SOL_PACKET:
		printxval(sockpacketoptions, name, "PACKET_???");
		break;
	case SOL_TCP:
		printxval(socktcpoptions, name, "TCP_???");
		break;
	case SOL_SCTP:
		printxval(socksctpoptions, name, "SCTP_???");
		break;
	case SOL_RAW:
		printxval(sockrawoptions, name, "RAW_???");
		break;
	case SOL_NETLINK:
		printxval(socknetlinkoptions, name, "NETLINK_???");
		break;

		/* Other SOL_* protocol levels still need work. */

	default:
		tprintf("%u", name);
	}

	tprints(", ");
}

static void
print_set_linger(struct tcb *const tcp, const kernel_ulong_t addr,
		 const int len)
{
	struct linger linger;

	if (len < (int) sizeof(linger)) {
		printaddr(addr);
	} else if (!umove_or_printaddr(tcp, addr, &linger)) {
		PRINT_FIELD_D("{", linger, l_onoff);
		PRINT_FIELD_D(", ", linger, l_linger);
		tprints("}");
	}
}

static void
print_get_linger(struct tcb *const tcp, const kernel_ulong_t addr,
		 unsigned int len)
{
	struct linger linger;

	if (len < sizeof(linger)) {
		if (len != sizeof(linger.l_onoff)) {
			printstr_ex(tcp, addr, len, QUOTE_FORCE_HEX);
			return;
		}
	} else {
		len = sizeof(linger);
	}

	if (umoven(tcp, addr, len, &linger) < 0) {
		printaddr(addr);
		return;
	}

	PRINT_FIELD_D("{", linger, l_onoff);
	if (len == sizeof(linger))
		PRINT_FIELD_D(", ", linger, l_linger);
	tprints("}");
}

#ifdef SO_PEERCRED
static void
print_ucred(struct tcb *const tcp, const kernel_ulong_t addr, unsigned int len)
{
	struct ucred uc;

	if (len < sizeof(uc)) {
		if (len != sizeof(uc.pid)
		    && len != offsetofend(struct ucred, uid)) {
			printstr_ex(tcp, addr, len, QUOTE_FORCE_HEX);
			return;
		}
	} else {
		len = sizeof(uc);
	}

	if (umoven(tcp, addr, len, &uc) < 0) {
		printaddr(addr);
		return;
	}

	PRINT_FIELD_D("{", uc, pid);
	if (len > sizeof(uc.pid))
		PRINT_FIELD_UID(", ", uc, uid);
	if (len == sizeof(uc))
		PRINT_FIELD_UID(", ", uc, gid);
	tprints("}");
}
#endif /* SO_PEERCRED */

#ifdef PACKET_STATISTICS
static void
print_tpacket_stats(struct tcb *const tcp, const kernel_ulong_t addr,
		    const int len)
{
	struct tpacket_stats stats;

	if (len != sizeof(stats) ||
	    umove(tcp, addr, &stats) < 0) {
		printaddr(addr);
	} else {
		PRINT_FIELD_U("{", stats, tp_packets);
		PRINT_FIELD_U("{", stats, tp_drops);
		tprints("}");
	}
}
#endif /* PACKET_STATISTICS */

#include "xlat/icmpfilterflags.h"

static void
print_icmp_filter(struct tcb *const tcp, const kernel_ulong_t addr, int len)
{
	struct icmp_filter filter = {};

	if (len > (int) sizeof(filter))
		len = sizeof(filter);
	else if (len <= 0) {
		printaddr(addr);
		return;
	}

	if (umoven_or_printaddr(tcp, addr, len, &filter))
		return;

	tprints("~(");
	printflags(icmpfilterflags, ~filter.data, "ICMP_???");
	tprints(")");
}

static bool
print_uint32(struct tcb *tcp, void *elem_buf, size_t elem_size, void *data)
{
	tprintf("%u", *(uint32_t *) elem_buf);

	return true;
}

static void
print_getsockopt(struct tcb *const tcp, const unsigned int level,
		 const unsigned int name, const kernel_ulong_t addr,
		 const int ulen, const int rlen)
{
	if (addr && verbose(tcp))
	switch (level) {
	case SOL_SOCKET:
		switch (name) {
		case SO_LINGER:
			print_get_linger(tcp, addr, rlen);
			return;
#ifdef SO_PEERCRED
		case SO_PEERCRED:
			print_ucred(tcp, addr, rlen);
			return;
#endif
#ifdef SO_ATTACH_FILTER
		case SO_ATTACH_FILTER:
			if (rlen && (unsigned short) rlen == (unsigned int) rlen)
				print_sock_fprog(tcp, addr, rlen);
			else
				printaddr(addr);
			return;
#endif /* SO_ATTACH_FILTER */
		}
		break;

	case SOL_PACKET:
		switch (name) {
#ifdef PACKET_STATISTICS
		case PACKET_STATISTICS:
			print_tpacket_stats(tcp, addr, rlen);
			return;
#endif
		}
		break;

	case SOL_RAW:
		switch (name) {
		case ICMP_FILTER:
			print_icmp_filter(tcp, addr, rlen);
			return;
		}
		break;

	case SOL_NETLINK:
		if (ulen < 0 || rlen < 0) {
			/*
			 * As the kernel neither accepts nor returns a negative
			 * length, in case of successful getsockopt syscall
			 * invocation these negative values must have come
			 * from userspace.
			 */
			printaddr(addr);
			return;
		}
		switch (name) {
		case NETLINK_LIST_MEMBERSHIPS: {
			uint32_t buf;
			print_array(tcp, addr, MIN(ulen, rlen) / sizeof(buf),
				    &buf, sizeof(buf),
				    umoven_or_printaddr, print_uint32, 0);
			break;
			}
		default:
			printnum_int(tcp, addr, "%d");
			break;
		}
		return;
	}

	/* default arg printing */

	if (verbose(tcp)) {
		if (rlen == sizeof(int)) {
			printnum_int(tcp, addr, "%d");
		} else {
			printstrn(tcp, addr, rlen);
		}
	} else {
		printaddr(addr);
	}
}

SYS_FUNC(getsockopt)
{
	int ulen, rlen;

	if (entering(tcp)) {
		print_sockopt_fd_level_name(tcp, tcp->u_arg[0],
					    tcp->u_arg[1], tcp->u_arg[2], true);

		if (verbose(tcp) && tcp->u_arg[4]
		    && umove(tcp, tcp->u_arg[4], &ulen) == 0) {
			set_tcb_priv_ulong(tcp, ulen);
			return 0;
		} else {
			printaddr(tcp->u_arg[3]);
			tprints(", ");
			printaddr(tcp->u_arg[4]);
			return RVAL_DECODED;
		}
	} else {
		ulen = get_tcb_priv_ulong(tcp);

		if (syserror(tcp) || umove(tcp, tcp->u_arg[4], &rlen) < 0) {
			printaddr(tcp->u_arg[3]);
			tprintf(", [%d]", ulen);
		} else {
			print_getsockopt(tcp, tcp->u_arg[1], tcp->u_arg[2],
					 tcp->u_arg[3], ulen, rlen);
			if (ulen != rlen)
				tprintf(", [%d->%d]", ulen, rlen);
			else
				tprintf(", [%d]", rlen);
		}
	}
	return 0;
}

#ifdef IP_ADD_MEMBERSHIP
static void
print_mreq(struct tcb *const tcp, const kernel_ulong_t addr,
	   const int len)
{
	struct ip_mreq mreq;

	if (len < (int) sizeof(mreq)) {
		printaddr(addr);
	} else if (!umove_or_printaddr(tcp, addr, &mreq)) {
		PRINT_FIELD_INET4_ADDR("{", mreq, imr_multiaddr);
		PRINT_FIELD_INET4_ADDR(", ", mreq, imr_interface);
		tprints("}");
	}
}
#endif /* IP_ADD_MEMBERSHIP */

#ifdef IPV6_ADD_MEMBERSHIP
static void
print_mreq6(struct tcb *const tcp, const kernel_ulong_t addr,
	    const int len)
{
	struct ipv6_mreq mreq;

	if (len < (int) sizeof(mreq)) {
		printaddr(addr);
	} else if (!umove_or_printaddr(tcp, addr, &mreq)) {
		PRINT_FIELD_INET_ADDR("{", mreq, ipv6mr_multiaddr, AF_INET6);
		PRINT_FIELD_IFINDEX(", ", mreq, ipv6mr_interface);
		tprints("}");
	}
}
#endif /* IPV6_ADD_MEMBERSHIP */

#ifdef PACKET_RX_RING
static void
print_tpacket_req(struct tcb *const tcp, const kernel_ulong_t addr, const int len)
{
	struct tpacket_req req;

	if (len != sizeof(req) ||
	    umove(tcp, addr, &req) < 0) {
		printaddr(addr);
	} else {
		PRINT_FIELD_U("{", req, tp_block_size);
		PRINT_FIELD_U(", ", req, tp_block_nr);
		PRINT_FIELD_U(", ", req, tp_frame_size);
		PRINT_FIELD_U(", ", req, tp_frame_nr);
		tprints("}");
	}
}
#endif /* PACKET_RX_RING */

#ifdef PACKET_ADD_MEMBERSHIP
# include "xlat/packet_mreq_type.h"

static void
print_packet_mreq(struct tcb *const tcp, const kernel_ulong_t addr, const int len)
{
	struct packet_mreq mreq;

	if (len != sizeof(mreq) ||
	    umove(tcp, addr, &mreq) < 0) {
		printaddr(addr);
	} else {
		unsigned int i;

		PRINT_FIELD_IFINDEX("{", mreq, mr_ifindex);
		PRINT_FIELD_XVAL(", ", mreq, mr_type, packet_mreq_type,
				 "PACKET_MR_???");
		PRINT_FIELD_U(", ", mreq, mr_alen);
		tprints(", mr_address=");
		if (mreq.mr_alen > ARRAY_SIZE(mreq.mr_address))
			mreq.mr_alen = ARRAY_SIZE(mreq.mr_address);
		for (i = 0; i < mreq.mr_alen; ++i)
			tprintf("%02x", mreq.mr_address[i]);
		tprints("}");
	}
}
#endif /* PACKET_ADD_MEMBERSHIP */

static void
print_setsockopt(struct tcb *const tcp, const unsigned int level,
		 const unsigned int name, const kernel_ulong_t addr,
		 const int len)
{
	if (addr && verbose(tcp))
	switch (level) {
	case SOL_SOCKET:
		switch (name) {
		case SO_LINGER:
			print_set_linger(tcp, addr, len);
			return;
#ifdef SO_ATTACH_FILTER
		case SO_ATTACH_FILTER:
# ifdef SO_ATTACH_REUSEPORT_CBPF
		case SO_ATTACH_REUSEPORT_CBPF:
# endif
			if ((unsigned int) len == get_sock_fprog_size())
				decode_sock_fprog(tcp, addr);
			else
				printaddr(addr);
			return;
#endif /* SO_ATTACH_FILTER */
		}
		break;

	case SOL_IP:
		switch (name) {
#ifdef IP_ADD_MEMBERSHIP
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
			print_mreq(tcp, addr, len);
			return;
#endif /* IP_ADD_MEMBERSHIP */
#ifdef MCAST_JOIN_GROUP
		case MCAST_JOIN_GROUP:
		case MCAST_LEAVE_GROUP:
			print_group_req(tcp, addr, len);
			return;
#endif /* MCAST_JOIN_GROUP */
		}
		break;

	case SOL_IPV6:
		switch (name) {
#ifdef IPV6_ADD_MEMBERSHIP
		case IPV6_ADD_MEMBERSHIP:
		case IPV6_DROP_MEMBERSHIP:
# ifdef IPV6_JOIN_ANYCAST
		case IPV6_JOIN_ANYCAST:
# endif
# ifdef IPV6_LEAVE_ANYCAST
		case IPV6_LEAVE_ANYCAST:
# endif
			print_mreq6(tcp, addr, len);
			return;
#endif /* IPV6_ADD_MEMBERSHIP */
#ifdef MCAST_JOIN_GROUP
		case MCAST_JOIN_GROUP:
		case MCAST_LEAVE_GROUP:
			print_group_req(tcp, addr, len);
			return;
#endif /* MCAST_JOIN_GROUP */
		}
		break;

	case SOL_PACKET:
		switch (name) {
#ifdef PACKET_RX_RING
		case PACKET_RX_RING:
# ifdef PACKET_TX_RING
		case PACKET_TX_RING:
# endif
			print_tpacket_req(tcp, addr, len);
			return;
#endif /* PACKET_RX_RING */
#ifdef PACKET_ADD_MEMBERSHIP
		case PACKET_ADD_MEMBERSHIP:
		case PACKET_DROP_MEMBERSHIP:
			print_packet_mreq(tcp, addr, len);
			return;
#endif /* PACKET_ADD_MEMBERSHIP */
		}
		break;

	case SOL_RAW:
		switch (name) {
		case ICMP_FILTER:
			print_icmp_filter(tcp, addr, len);
			return;
		}
		break;

	case SOL_NETLINK:
		if (len < (int) sizeof(int))
			printaddr(addr);
		else
			printnum_int(tcp, addr, "%d");
		return;
	}

	/* default arg printing */

	if (verbose(tcp)) {
		if (len == sizeof(int)) {
			printnum_int(tcp, addr, "%d");
		} else {
			printstrn(tcp, addr, len);
		}
	} else {
		printaddr(addr);
	}
}

SYS_FUNC(setsockopt)
{
	print_sockopt_fd_level_name(tcp, tcp->u_arg[0],
				    tcp->u_arg[1], tcp->u_arg[2], false);
	print_setsockopt(tcp, tcp->u_arg[1], tcp->u_arg[2],
			 tcp->u_arg[3], tcp->u_arg[4]);
	tprintf(", %d", (int) tcp->u_arg[4]);

	return RVAL_DECODED;
}
