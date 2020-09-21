/*
 * Copyright (c) 2016 Fabien Siron <fabien.siron@epita.fr>
 * Copyright (c) 2017 JingPiao Chen <chenjingpiao@gmail.com>
 * Copyright (c) 2016-2017 The strace developers.
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
#include "netlink.h"
#include "netlink_route.h"

#include <linux/rtnetlink.h>

#include "xlat/nl_route_types.h"

static void
decode_family(struct tcb *const tcp, const uint8_t family,
	      const kernel_ulong_t addr, const unsigned int len)
{
	tprints("{family=");
	printxval(addrfams, family, "AF_???");
	if (len > sizeof(family)) {
		tprints(", ");
		printstr_ex(tcp, addr + sizeof(family),
			    len - sizeof(family), QUOTE_FORCE_HEX);
	}
	tprints("}");
}

typedef DECL_NETLINK_ROUTE_DECODER((*netlink_route_decoder_t));

static const netlink_route_decoder_t route_decoders[] = {
	[RTM_DELLINK - RTM_BASE] = decode_ifinfomsg,
	[RTM_GETLINK - RTM_BASE] = decode_ifinfomsg,
	[RTM_NEWLINK - RTM_BASE] = decode_ifinfomsg,
	[RTM_SETLINK - RTM_BASE] = decode_ifinfomsg,

	[RTM_DELADDR - RTM_BASE] = decode_ifaddrmsg,
	[RTM_GETADDR - RTM_BASE] = decode_ifaddrmsg,
	[RTM_GETANYCAST - RTM_BASE] = decode_ifaddrmsg,
	[RTM_GETMULTICAST - RTM_BASE] = decode_ifaddrmsg,
	[RTM_NEWADDR - RTM_BASE] = decode_ifaddrmsg,

	[RTM_DELROUTE - RTM_BASE] = decode_rtmsg,
	[RTM_GETROUTE - RTM_BASE] = decode_rtmsg,
	[RTM_NEWROUTE - RTM_BASE] = decode_rtmsg,

	[RTM_DELRULE - RTM_BASE] = decode_fib_rule_hdr,
	[RTM_GETRULE - RTM_BASE] = decode_fib_rule_hdr,
	[RTM_NEWRULE - RTM_BASE] = decode_fib_rule_hdr,

	[RTM_DELNEIGH - RTM_BASE] = decode_ndmsg,
	[RTM_GETNEIGH - RTM_BASE] = decode_rtm_getneigh,
	[RTM_NEWNEIGH - RTM_BASE] = decode_ndmsg,

	[RTM_GETNEIGHTBL - RTM_BASE] = decode_ndtmsg,
	[RTM_NEWNEIGHTBL - RTM_BASE] = decode_ndtmsg,
	[RTM_SETNEIGHTBL - RTM_BASE] = decode_ndtmsg,

	[RTM_DELQDISC - RTM_BASE] = decode_tcmsg,
	[RTM_GETQDISC - RTM_BASE] = decode_tcmsg,
	[RTM_NEWQDISC - RTM_BASE] = decode_tcmsg,
	[RTM_DELTCLASS - RTM_BASE] = decode_tcmsg,
	[RTM_GETTCLASS - RTM_BASE] = decode_tcmsg,
	[RTM_NEWTCLASS - RTM_BASE] = decode_tcmsg,
	[RTM_DELTFILTER - RTM_BASE] = decode_tcmsg,
	[RTM_GETTFILTER - RTM_BASE] = decode_tcmsg,
	[RTM_NEWTFILTER - RTM_BASE] = decode_tcmsg,

	[RTM_DELACTION - RTM_BASE] = decode_tcamsg,
	[RTM_GETACTION - RTM_BASE] = decode_tcamsg,
	[RTM_NEWACTION - RTM_BASE] = decode_tcamsg,

#ifdef HAVE_STRUCT_IFADDRLBLMSG
	[RTM_DELADDRLABEL - RTM_BASE] = decode_ifaddrlblmsg,
	[RTM_GETADDRLABEL - RTM_BASE] = decode_ifaddrlblmsg,
	[RTM_NEWADDRLABEL - RTM_BASE] = decode_ifaddrlblmsg,
#endif

#ifdef HAVE_STRUCT_DCBMSG
	[RTM_GETDCB - RTM_BASE] = decode_dcbmsg,
	[RTM_SETDCB - RTM_BASE] = decode_dcbmsg,
#endif

#ifdef HAVE_STRUCT_NETCONFMSG
	[RTM_DELNETCONF - RTM_BASE] = decode_netconfmsg,
	[RTM_GETNETCONF - RTM_BASE] = decode_netconfmsg,
	[RTM_NEWNETCONF - RTM_BASE] = decode_netconfmsg,
#endif

#ifdef HAVE_STRUCT_BR_PORT_MSG
	[RTM_DELMDB - RTM_BASE] = decode_br_port_msg,
	[RTM_GETMDB - RTM_BASE] = decode_br_port_msg,
	[RTM_NEWMDB - RTM_BASE] = decode_br_port_msg,
#endif

	[RTM_DELNSID - RTM_BASE] = decode_rtgenmsg,
	[RTM_GETNSID - RTM_BASE] = decode_rtgenmsg,
	[RTM_NEWNSID - RTM_BASE] = decode_rtgenmsg
};

bool
decode_netlink_route(struct tcb *const tcp,
		     const struct nlmsghdr *const nlmsghdr,
		     const kernel_ulong_t addr,
		     const unsigned int len)
{
	uint8_t family;

	if (nlmsghdr->nlmsg_type == NLMSG_DONE)
		return false;

	if (!umove_or_printaddr(tcp, addr, &family)) {
		const unsigned int index = nlmsghdr->nlmsg_type - RTM_BASE;

		if (index < ARRAY_SIZE(route_decoders)
		    && route_decoders[index]) {
			route_decoders[index](tcp, nlmsghdr, family, addr, len);
		} else {
			decode_family(tcp, family, addr, len);
		}
	}

	return true;
}
