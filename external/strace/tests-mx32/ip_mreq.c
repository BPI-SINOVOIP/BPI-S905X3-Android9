/*
 * Copyright (c) 2015-2016 Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (c) 2015-2018 The strace developers.
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
#include <netinet/in.h>

#if defined IP_ADD_MEMBERSHIP && defined IPV6_ADD_MEMBERSHIP \
 && defined IPV6_JOIN_ANYCAST

# include <limits.h>
# include <stdio.h>
# include <unistd.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <net/if.h>

#define	multi4addr	"224.0.0.3"
#define	multi6addr	"ff01::c"
#define	interface	"127.0.0.1"

int
main(void)
{
	TAIL_ALLOC_OBJECT_CONST_PTR(struct ip_mreq, m4);
	TAIL_ALLOC_OBJECT_CONST_PTR(struct ipv6_mreq, m6);
	unsigned int i;
	int rc;

	inet_pton(AF_INET, multi4addr, &m4->imr_multiaddr);
	inet_pton(AF_INET, interface, &m4->imr_interface);
	inet_pton(AF_INET6, multi6addr, &m6->ipv6mr_multiaddr);

	m6->ipv6mr_interface = ifindex_lo();
	if (!m6->ipv6mr_interface)
		perror_msg_and_skip("lo");

	(void) close(0);
	if (socket(AF_INET, SOCK_DGRAM, 0))
		perror_msg_and_skip("socket");

	struct {
		const int level;
		const char *const str_level;
		const int name;
		const char *str_name;
		const void *const val;
		unsigned int size;
		const char *const addr;
	} opts[] = {
		{
			ARG_STR(SOL_IP), ARG_STR(IP_ADD_MEMBERSHIP),
			m4, sizeof(*m4),
			"{imr_multiaddr=inet_addr(\"" multi4addr
			"\"), imr_interface=inet_addr(\"" interface "\")}"
		},
		{
			ARG_STR(SOL_IP), ARG_STR(IP_DROP_MEMBERSHIP),
			m4, sizeof(*m4),
			"{imr_multiaddr=inet_addr(\"" multi4addr
			"\"), imr_interface=inet_addr(\"" interface "\")}"
		},
		{
			ARG_STR(SOL_IPV6), ARG_STR(IPV6_ADD_MEMBERSHIP),
			m6, sizeof(*m6),
			"{inet_pton(AF_INET6, \"" multi6addr
			"\", &ipv6mr_multiaddr)"
			", ipv6mr_interface=" IFINDEX_LO_STR "}"
		},
		{
			ARG_STR(SOL_IPV6), ARG_STR(IPV6_DROP_MEMBERSHIP),
			m6, sizeof(*m6),
			"{inet_pton(AF_INET6, \"" multi6addr
			"\", &ipv6mr_multiaddr)"
			", ipv6mr_interface=" IFINDEX_LO_STR "}"
		},
		{
			ARG_STR(SOL_IPV6), ARG_STR(IPV6_JOIN_ANYCAST),
			m6, sizeof(*m6),
			"{inet_pton(AF_INET6, \"" multi6addr
			"\", &ipv6mr_multiaddr)"
			", ipv6mr_interface=" IFINDEX_LO_STR "}"
		},
		{
			ARG_STR(SOL_IPV6), ARG_STR(IPV6_LEAVE_ANYCAST),
			m6, sizeof(*m6),
			"{inet_pton(AF_INET6, \"" multi6addr
			"\", &ipv6mr_multiaddr)"
			", ipv6mr_interface=" IFINDEX_LO_STR "}"
		}
	};

	for (i = 0; i < ARRAY_SIZE(opts); ++i) {
		/* optlen < 0, EINVAL */
		rc = setsockopt(0, opts[i].level, opts[i].name,
				opts[i].val, -1);
		printf("setsockopt(0, %s, %s, %p, -1) = %s\n",
		       opts[i].str_level, opts[i].str_name,
		       opts[i].val, sprintrc(rc));

		/* optlen < sizeof(struct), EINVAL */
		rc = setsockopt(0, opts[i].level, opts[i].name,
				opts[i].val, opts[i].size - 1);
		printf("setsockopt(0, %s, %s, %p, %u) = %s\n",
		       opts[i].str_level, opts[i].str_name,
		       opts[i].val, opts[i].size - 1, sprintrc(rc));

		/* optval EFAULT */
		rc = setsockopt(0, opts[i].level, opts[i].name,
				opts[i].val + 1, opts[i].size);
		printf("setsockopt(0, %s, %s, %p, %u) = %s\n",
		       opts[i].str_level, opts[i].str_name,
		       opts[i].val + 1, opts[i].size, sprintrc(rc));

		/* classic */
		rc = setsockopt(0, opts[i].level, opts[i].name,
				opts[i].val, opts[i].size);
		printf("setsockopt(0, %s, %s, %s, %u) = %s\n",
		       opts[i].str_level, opts[i].str_name,
		       opts[i].addr, opts[i].size, sprintrc(rc));

		/* optlen > sizeof(struct), shortened */
		rc = setsockopt(0, opts[i].level, opts[i].name,
				opts[i].val, INT_MAX);
		printf("setsockopt(0, %s, %s, %s, %u) = %s\n",
		       opts[i].str_level, opts[i].str_name,
		       opts[i].addr, INT_MAX, sprintrc(rc));
	}

	puts("+++ exited with 0 +++");
	return 0;
}

#else

SKIP_MAIN_UNDEFINED("IP_ADD_MEMBERSHIP && IPV6_ADD_MEMBERSHIP"
		    " && IPV6_JOIN_ANYCAST")

#endif
