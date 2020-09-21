/*
 * Copyright (c) 1993, 1994, 1995, 1996 Rick Sladkey <jrs@world.std.com>
 * Copyright (c) 1996-2017 The strace developers.
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

#include <sys/socket.h>
#if defined ALPHA || defined SH || defined SH64
# include <linux/ioctl.h>
#endif
#include <linux/sockios.h>
#include <arpa/inet.h>
#include <net/if.h>

#include DEF_MPERS_TYPE(struct_ifconf)
#include DEF_MPERS_TYPE(struct_ifreq)

typedef struct ifconf struct_ifconf;
typedef struct ifreq struct_ifreq;

#include MPERS_DEFS

#include "xlat/iffflags.h"

static void
print_ifname(const char *ifname)
{
	print_quoted_string(ifname, IFNAMSIZ + 1, QUOTE_0_TERMINATED);
}

static void
print_ifreq(struct tcb *const tcp, const unsigned int code,
	    const kernel_ulong_t arg, const struct_ifreq *const ifr)
{
	switch (code) {
	case SIOCSIFADDR:
	case SIOCGIFADDR:
		PRINT_FIELD_SOCKADDR("", *ifr, ifr_addr);
		break;
	case SIOCSIFDSTADDR:
	case SIOCGIFDSTADDR:
		PRINT_FIELD_SOCKADDR("", *ifr, ifr_dstaddr);
		break;
	case SIOCSIFBRDADDR:
	case SIOCGIFBRDADDR:
		PRINT_FIELD_SOCKADDR("", *ifr, ifr_broadaddr);
		break;
	case SIOCSIFNETMASK:
	case SIOCGIFNETMASK:
		PRINT_FIELD_SOCKADDR("", *ifr, ifr_netmask);
		break;
	case SIOCSIFHWADDR:
	case SIOCGIFHWADDR: {
		/* XXX Are there other hardware addresses
		   than 6-byte MACs?  */
		const unsigned char *bytes =
			(unsigned char *) &ifr->ifr_hwaddr.sa_data;
		tprintf("ifr_hwaddr=%02x:%02x:%02x:%02x:%02x:%02x",
			bytes[0], bytes[1], bytes[2],
			bytes[3], bytes[4], bytes[5]);
		break;
	}
	case SIOCSIFFLAGS:
	case SIOCGIFFLAGS:
		tprints("ifr_flags=");
		printflags(iffflags, (unsigned short) ifr->ifr_flags, "IFF_???");
		break;
	case SIOCSIFMETRIC:
	case SIOCGIFMETRIC:
		tprintf("ifr_metric=%d", ifr->ifr_metric);
		break;
	case SIOCSIFMTU:
	case SIOCGIFMTU:
		tprintf("ifr_mtu=%d", ifr->ifr_mtu);
		break;
	case SIOCSIFSLAVE:
	case SIOCGIFSLAVE:
		tprints("ifr_slave=");
		print_ifname(ifr->ifr_slave);
		break;
	case SIOCSIFTXQLEN:
	case SIOCGIFTXQLEN:
		tprintf("ifr_qlen=%d", ifr->ifr_qlen);
		break;
	case SIOCSIFMAP:
	case SIOCGIFMAP:
		tprintf("ifr_map={mem_start=%#" PRI_klx ", "
			"mem_end=%#" PRI_klx ", base_addr=%#x, "
			"irq=%u, dma=%u, port=%u}",
			(kernel_ulong_t) ifr->ifr_map.mem_start,
			(kernel_ulong_t) ifr->ifr_map.mem_end,
			(unsigned) ifr->ifr_map.base_addr,
			(unsigned) ifr->ifr_map.irq,
			(unsigned) ifr->ifr_map.dma,
			(unsigned) ifr->ifr_map.port);
		break;
	}
}

static unsigned int
print_ifc_len(int len)
{
	const unsigned int n = (unsigned int) len / sizeof(struct_ifreq);

	if (len < 0 || n * sizeof(struct_ifreq) != (unsigned int) len)
		tprintf("%d", len);
	else
		tprintf("%u * sizeof(struct ifreq)", n);

	return n;
}

static bool
print_ifconf_ifreq(struct tcb *tcp, void *elem_buf, size_t elem_size,
		   void *dummy)
{
	struct_ifreq *ifr = elem_buf;

	tprints("{ifr_name=");
	print_ifname(ifr->ifr_name);
	PRINT_FIELD_SOCKADDR(", ", *ifr, ifr_addr);
	tprints("}");

	return true;
}

/*
 * There are two different modes of operation:
 *
 * - Get buffer size.  In this case, the callee sets ifc_buf to NULL,
 *   and the kernel returns the buffer size in ifc_len.
 * - Get actual data.  In this case, the callee specifies the buffer address
 *   in ifc_buf and its size in ifc_len.  The kernel fills the buffer with
 *   the data, and its amount is returned in ifc_len.
 *
 * Note that, technically, the whole struct ifconf is overwritten,
 * so ifc_buf could be different on exit, but current ioctl handler
 * implementation does not touch it.
 */
static int
decode_ifconf(struct tcb *const tcp, const kernel_ulong_t addr)
{
	struct_ifconf *entering_ifc = NULL;
	struct_ifconf *ifc =
		entering(tcp) ? malloc(sizeof(*ifc)) : alloca(sizeof(*ifc));

	if (exiting(tcp)) {
		entering_ifc = get_tcb_priv_data(tcp);

		if (!entering_ifc) {
			error_msg("decode_ifconf: where is my ifconf?");
			return 0;
		}
	}

	if (!ifc || umove(tcp, addr, ifc) < 0) {
		if (entering(tcp)) {
			free(ifc);

			tprints(", ");
			printaddr(addr);
		} else {
			/*
			 * We failed to fetch the structure on exiting syscall,
			 * print whatever was fetched on entering syscall.
			 */
			if (!entering_ifc->ifc_buf)
				print_ifc_len(entering_ifc->ifc_len);

			tprints(", ifc_buf=");
			printaddr(ptr_to_kulong(entering_ifc->ifc_buf));

			tprints("}");
		}

		return RVAL_IOCTL_DECODED;
	}

	if (entering(tcp)) {
		tprints(", {ifc_len=");
		if (ifc->ifc_buf)
			print_ifc_len(ifc->ifc_len);

		set_tcb_priv_data(tcp, ifc, free);

		return 0;
	}

	/* exiting */

	if (entering_ifc->ifc_buf && (entering_ifc->ifc_len != ifc->ifc_len))
		tprints(" => ");
	if (!entering_ifc->ifc_buf || (entering_ifc->ifc_len != ifc->ifc_len))
		print_ifc_len(ifc->ifc_len);

	tprints(", ifc_buf=");

	if (!entering_ifc->ifc_buf || syserror(tcp)) {
		printaddr(ptr_to_kulong(entering_ifc->ifc_buf));
		if (entering_ifc->ifc_buf != ifc->ifc_buf) {
			tprints(" => ");
			printaddr(ptr_to_kulong(ifc->ifc_buf));
		}
	} else {
		struct_ifreq ifr;

		print_array(tcp, ptr_to_kulong(ifc->ifc_buf),
			    ifc->ifc_len / sizeof(struct_ifreq),
			    &ifr, sizeof(ifr),
			    umoven_or_printaddr, print_ifconf_ifreq, NULL);
	}

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

MPERS_PRINTER_DECL(int, sock_ioctl,
		   struct tcb *tcp, const unsigned int code,
		   const kernel_ulong_t arg)
{
	struct_ifreq ifr;

	switch (code) {
	case SIOCGIFCONF:
		return decode_ifconf(tcp, arg);

#ifdef SIOCBRADDBR
	case SIOCBRADDBR:
	case SIOCBRDELBR:
		tprints(", ");
		printstr(tcp, arg);
		break;
#endif

#ifdef FIOSETOWN
	case FIOSETOWN:
#endif
#ifdef SIOCSPGRP
	case SIOCSPGRP:
#endif
		tprints(", ");
		printnum_int(tcp, arg, "%d");
		break;

#ifdef FIOGETOWN
	case FIOGETOWN:
#endif
#ifdef SIOCGPGRP
	case SIOCGPGRP:
#endif
#ifdef SIOCATMARK
	case SIOCATMARK:
#endif
		if (entering(tcp))
			return 0;
		tprints(", ");
		printnum_int(tcp, arg, "%d");
		break;

#ifdef SIOCBRADDIF
	case SIOCBRADDIF:
#endif
#ifdef SIOCBRDELIF
	case SIOCBRDELIF:
#endif
		/* no arguments */
		break;

	case SIOCSIFNAME:
	case SIOCSIFADDR:
	case SIOCSIFDSTADDR:
	case SIOCSIFBRDADDR:
	case SIOCSIFNETMASK:
	case SIOCSIFFLAGS:
	case SIOCSIFMETRIC:
	case SIOCSIFMTU:
	case SIOCSIFSLAVE:
	case SIOCSIFHWADDR:
	case SIOCSIFTXQLEN:
	case SIOCSIFMAP:
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &ifr))
			break;

		tprints("{ifr_name=");
		print_ifname(ifr.ifr_name);
		tprints(", ");
		if (code == SIOCSIFNAME) {
			tprints("ifr_newname=");
			print_ifname(ifr.ifr_newname);
		} else {
			print_ifreq(tcp, code, arg, &ifr);
		}
		tprints("}");
		break;

	case SIOCGIFNAME:
	case SIOCGIFINDEX:
	case SIOCGIFADDR:
	case SIOCGIFDSTADDR:
	case SIOCGIFBRDADDR:
	case SIOCGIFNETMASK:
	case SIOCGIFFLAGS:
	case SIOCGIFMETRIC:
	case SIOCGIFMTU:
	case SIOCGIFSLAVE:
	case SIOCGIFHWADDR:
	case SIOCGIFTXQLEN:
	case SIOCGIFMAP:
		if (entering(tcp)) {
			tprints(", ");
			if (umove_or_printaddr(tcp, arg, &ifr))
				break;

			if (SIOCGIFNAME == code) {
				tprintf("{ifr_index=%d", ifr.ifr_ifindex);
			} else {
				tprints("{ifr_name=");
				print_ifname(ifr.ifr_name);
			}
			return 0;
		} else {
			if (syserror(tcp)) {
				tprints("}");
				break;
			}

			tprints(", ");
			if (umove(tcp, arg, &ifr) < 0) {
				tprints("???}");
				break;
			}

			if (SIOCGIFNAME == code) {
				tprints("ifr_name=");
				print_ifname(ifr.ifr_name);
			} else {
				print_ifreq(tcp, code, arg, &ifr);
			}
			tprints("}");
			break;
		}

	default:
		return RVAL_DECODED;
	}

	return RVAL_IOCTL_DECODED;
}
