/*
 * Copyright (c) 2017 JingPiao Chen <chenjingpiao@gmail.com>
 * Copyright (c) 2017 The strace developers.
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

#include "tests.h"

#include <stdio.h>
#include "test_nlattr.h"
#include <linux/if.h>
#include <linux/if_arp.h>
#ifdef HAVE_LINUX_IF_LINK_H
# include <linux/if_link.h>
#endif
#include <linux/rtnetlink.h>

#ifndef IFLA_PORT_SELF
# define IFLA_PORT_SELF 25
#endif
#ifndef IFLA_PORT_VF
# define IFLA_PORT_VF 1
#endif

const unsigned int hdrlen = sizeof(struct ifinfomsg);

static void
init_ifinfomsg(struct nlmsghdr *const nlh, const unsigned int msg_len)
{
	SET_STRUCT(struct nlmsghdr, nlh,
		.nlmsg_len = msg_len,
		.nlmsg_type = RTM_GETLINK,
		.nlmsg_flags = NLM_F_DUMP
	);

	struct ifinfomsg *const msg = NLMSG_DATA(nlh);
	SET_STRUCT(struct ifinfomsg, msg,
		.ifi_family = AF_UNIX,
		.ifi_type = ARPHRD_LOOPBACK,
		.ifi_index = ifindex_lo(),
		.ifi_flags = IFF_UP,
	);

	struct nlattr *const nla = NLMSG_ATTR(nlh, sizeof(*msg));
	SET_STRUCT(struct nlattr, nla,
		.nla_len = msg_len - NLMSG_SPACE(hdrlen),
		.nla_type = IFLA_PORT_SELF
	);
}

static void
print_ifinfomsg(const unsigned int msg_len)
{
	printf("{len=%u, type=RTM_GETLINK, flags=NLM_F_DUMP"
	       ", seq=0, pid=0}, {ifi_family=AF_UNIX"
	       ", ifi_type=ARPHRD_LOOPBACK"
	       ", ifi_index=" IFINDEX_LO_STR
	       ", ifi_flags=IFF_UP, ifi_change=0}"
	       ", {{nla_len=%u, nla_type=IFLA_PORT_SELF}",
	       msg_len, msg_len - NLMSG_SPACE(hdrlen));
}

int
main(void)
{
	skip_if_unavailable("/proc/self/fd/");

	const int fd = create_nl_socket(NETLINK_ROUTE);
	void *nlh0 = tail_alloc(NLMSG_SPACE(hdrlen));

	static char pattern[4096];
	fill_memory_ex(pattern, sizeof(pattern), 'a', 'z' - 'a' + 1);

	const uint32_t num = 0xabacdbcd;
	TEST_NESTED_NLATTR_OBJECT(fd, nlh0, hdrlen,
				  init_ifinfomsg, print_ifinfomsg,
				  IFLA_PORT_VF, pattern, num,
				  printf("%u", num));

#ifdef HAVE_STRUCT_IFLA_PORT_VSI
	static const struct ifla_port_vsi vsi = {
		.vsi_mgr_id = 0xab,
		.vsi_type_id = "abc",
		.vsi_type_version = 0xef
	};
	TEST_NESTED_NLATTR_OBJECT(fd, nlh0, hdrlen,
				  init_ifinfomsg, print_ifinfomsg,
				  IFLA_PORT_VSI_TYPE, pattern, vsi,
				  PRINT_FIELD_U("{", vsi, vsi_mgr_id);
				  printf(", vsi_type_id=\"\\x61\\x62\\x63\"");
				  PRINT_FIELD_U(", ", vsi, vsi_type_version);
				  printf("}"));
#endif

	puts("+++ exited with 0 +++");
	return 0;
}
