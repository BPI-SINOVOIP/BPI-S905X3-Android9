/*
 * Check decoding of MCAST_JOIN_GROUP/MCAST_LEAVE_GROUP.
 *
 * Copyright (c) 2015-2017 Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (c) 2017-2018 The strace developers.
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
#include <net/if.h>
#include <netinet/in.h>

#if defined MCAST_JOIN_GROUP && defined MCAST_LEAVE_GROUP

# include <limits.h>
# include <stdio.h>
# include <unistd.h>
# include <sys/socket.h>
# include <arpa/inet.h>

#define	multi4addr	"224.0.0.3"
#define	multi6addr	"ff01::c"

static const char *errstr;

static int
set_opt(const int fd, const int level, const int opt,
	const void *const val, const socklen_t len)
{
	int rc = setsockopt(fd, level, opt, val, len);
	errstr = sprintrc(rc);
	return rc;
}

int
main(void)
{
	TAIL_ALLOC_OBJECT_CONST_PTR(struct group_req, greq4);
	TAIL_ALLOC_OBJECT_CONST_PTR(struct group_req, greq6);
	unsigned int i;

	greq6->gr_interface = greq4->gr_interface = ifindex_lo();
	if (!greq4->gr_interface)
		perror_msg_and_skip("lo");

	greq4->gr_group.ss_family = AF_INET;
	inet_pton(AF_INET, multi4addr, &greq4->gr_group.ss_family + 2);

	greq6->gr_group.ss_family = AF_INET6;
	inet_pton(AF_INET6, multi6addr, &greq6->gr_group.ss_family + 4);

	(void) close(0);
	if (socket(AF_INET, SOCK_DGRAM, 0))
		perror_msg_and_skip("socket");

	struct {
		const int level;
		const char *const str_level;
		const int name;
		const char *const str_name;
		const struct group_req *const val;
		const char *const addr;
	} opts[] = {
		{
			ARG_STR(SOL_IP), ARG_STR(MCAST_JOIN_GROUP), greq4,
			"gr_group={sa_family=AF_INET, sin_port=htons(65535)"
			", sin_addr=inet_addr(\"" multi4addr "\")}"
		},
		{
			ARG_STR(SOL_IP), ARG_STR(MCAST_LEAVE_GROUP), greq4,
			"gr_group={sa_family=AF_INET, sin_port=htons(65535)"
			", sin_addr=inet_addr(\"" multi4addr "\")}"
		},
		{
			ARG_STR(SOL_IPV6), ARG_STR(MCAST_JOIN_GROUP), greq6,
			"gr_group={sa_family=AF_INET6, sin6_port=htons(65535)"
			", inet_pton(AF_INET6, \"" multi6addr "\", &sin6_addr)"
			", sin6_flowinfo=htonl(4294967295)"
			", sin6_scope_id=4294967295}"
		},
		{
			ARG_STR(SOL_IPV6), ARG_STR(MCAST_LEAVE_GROUP), greq6,
			"gr_group={sa_family=AF_INET6, sin6_port=htons(65535)"
			", inet_pton(AF_INET6, \"" multi6addr "\", &sin6_addr)"
			", sin6_flowinfo=htonl(4294967295)"
			", sin6_scope_id=4294967295}"
		}
	};

	for (i = 0; i < ARRAY_SIZE(opts); ++i) {
		/* optlen < 0, EINVAL */
		set_opt(0, opts[i].level, opts[i].name, opts[i].val, -1U);
		printf("setsockopt(0, %s, %s, %p, -1) = %s\n",
		       opts[i].str_level, opts[i].str_name,
		       opts[i].val, errstr);

		/* optlen < sizeof(struct group_req), EINVAL */
		set_opt(0, opts[i].level, opts[i].name, opts[i].val,
			sizeof(*opts[i].val) - 1);
		printf("setsockopt(0, %s, %s, %p, %u) = %s\n",
		       opts[i].str_level, opts[i].str_name,
		       opts[i].val, (unsigned int) sizeof(*opts[i].val) - 1,
		       errstr);

		/* optval EFAULT */
		set_opt(0, opts[i].level, opts[i].name,
			(const char *) opts[i].val + 1, sizeof(*opts[i].val));
		printf("setsockopt(0, %s, %s, %p, %u) = %s\n",
		       opts[i].str_level, opts[i].str_name,
		       (const char *) opts[i].val + 1,
		       (unsigned int) sizeof(*opts[i].val), errstr);

		/* classic */
		set_opt(0, opts[i].level, opts[i].name,
			opts[i].val, sizeof(*opts[i].val));
		printf("setsockopt(0, %s, %s"
		       ", {gr_interface=%s, %s}, %u) = %s\n",
		       opts[i].str_level, opts[i].str_name,
		       IFINDEX_LO_STR, opts[i].addr,
		       (unsigned int) sizeof(*opts[i].val), errstr);

		/* optlen > sizeof(struct group_req), shortened */
		set_opt(0, opts[i].level, opts[i].name, opts[i].val, INT_MAX);
		printf("setsockopt(0, %s, %s"
		       ", {gr_interface=%s, %s}, %u) = %s\n",
		       opts[i].str_level, opts[i].str_name,
		       IFINDEX_LO_STR, opts[i].addr,
		       INT_MAX, errstr);
	}

	puts("+++ exited with 0 +++");
	return 0;
}

#else

SKIP_MAIN_UNDEFINED("MCAST_JOIN_GROUP && MCAST_LEAVE_GROUP")

#endif
