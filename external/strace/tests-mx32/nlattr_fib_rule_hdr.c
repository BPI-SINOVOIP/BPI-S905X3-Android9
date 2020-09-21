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

#ifdef HAVE_LINUX_FIB_RULES_H

# include <stdio.h>
# include <inttypes.h>
# include "test_nlattr.h"
# include <linux/fib_rules.h>
# include <linux/ip.h>
# include <linux/rtnetlink.h>

#define FRA_TUN_ID 12
#define FRA_TABLE 15
#define FRA_UID_RANGE 20

static void
init_rtmsg(struct nlmsghdr *const nlh, const unsigned int msg_len)
{
	SET_STRUCT(struct nlmsghdr, nlh,
		.nlmsg_len = msg_len,
		.nlmsg_type = RTM_GETRULE,
		.nlmsg_flags = NLM_F_DUMP
	);

	struct rtmsg *const msg = NLMSG_DATA(nlh);
	SET_STRUCT(struct rtmsg, msg,
		.rtm_family = AF_UNIX,
		.rtm_tos = IPTOS_LOWDELAY,
		.rtm_table = RT_TABLE_UNSPEC,
		.rtm_type = FR_ACT_TO_TBL,
		.rtm_flags = FIB_RULE_INVERT
	);
}

static void
print_rtmsg(const unsigned int msg_len)
{
	printf("{len=%u, type=RTM_GETRULE, flags=NLM_F_DUMP"
	       ", seq=0, pid=0}, {family=AF_UNIX"
	       ", dst_len=0, src_len=0"
	       ", tos=IPTOS_LOWDELAY"
	       ", table=RT_TABLE_UNSPEC"
	       ", action=FR_ACT_TO_TBL"
	       ", flags=FIB_RULE_INVERT}",
	       msg_len);
}

int
main(void)
{
	skip_if_unavailable("/proc/self/fd/");

	const int fd = create_nl_socket(NETLINK_ROUTE);
	const unsigned int hdrlen = sizeof(struct rtmsg);
	void *nlh0 = tail_alloc(NLMSG_SPACE(hdrlen));

	static char pattern[4096];
	fill_memory_ex(pattern, sizeof(pattern), 'a', 'z' - 'a' + 1);

	const unsigned int nla_type = 0xffff & NLA_TYPE_MASK;
	char nla_type_str[256];
	sprintf(nla_type_str, "%#x /* FRA_??? */", nla_type);
	TEST_NLATTR_(fd, nlh0, hdrlen,
		     init_rtmsg, print_rtmsg,
		     nla_type, nla_type_str,
		     4, pattern, 4,
		     print_quoted_hex(pattern, 4));

	TEST_NLATTR(fd, nlh0, hdrlen,
		    init_rtmsg, print_rtmsg,
		    FRA_DST, 4, pattern, 4,
		    print_quoted_hex(pattern, 4));

	const uint32_t table_id = RT_TABLE_DEFAULT;
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_rtmsg, print_rtmsg,
			   FRA_TABLE, pattern, table_id,
			   printf("RT_TABLE_DEFAULT"));

#ifdef HAVE_STRUCT_FIB_RULE_UID_RANGE
	static const struct fib_rule_uid_range range = {
		.start = 0xabcdedad,
		.end = 0xbcdeadba
	};
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_rtmsg, print_rtmsg,
			   FRA_UID_RANGE, pattern, range,
			   PRINT_FIELD_U("{", range, start);
			   PRINT_FIELD_U(", ", range, end);
			   printf("}"));
#endif
#if defined HAVE_BE64TOH || defined be64toh
	const uint64_t tun_id = 0xabcdcdbeedabadef;
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_rtmsg, print_rtmsg,
			   FRA_TUN_ID, pattern, tun_id,
			   printf("htobe64(%" PRIu64 ")", be64toh(tun_id)));
#endif

	puts("+++ exited with 0 +++");
	return 0;
}

#else

SKIP_MAIN_UNDEFINED("HAVE_LINUX_FIB_RULES_H")

#endif
