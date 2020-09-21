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
#include <netinet/in.h>
#include <arpa/inet.h>
#include "test_nlattr.h"
#include <linux/ip.h>
#include <linux/rtnetlink.h>

#define RTA_ENCAP_TYPE 21
#define LWTUNNEL_ENCAP_NONE 0

static void
init_rtmsg(struct nlmsghdr *const nlh, const unsigned int msg_len)
{
	SET_STRUCT(struct nlmsghdr, nlh,
		.nlmsg_len = msg_len,
		.nlmsg_type = RTM_GETROUTE,
		.nlmsg_flags = NLM_F_DUMP
	);

	struct rtmsg *const msg = NLMSG_DATA(nlh);
	SET_STRUCT(struct rtmsg, msg,
		.rtm_family = AF_UNIX,
		.rtm_tos = IPTOS_LOWDELAY,
		.rtm_table = RT_TABLE_DEFAULT,
		.rtm_protocol = RTPROT_KERNEL,
		.rtm_scope = RT_SCOPE_UNIVERSE,
		.rtm_type = RTN_LOCAL,
		.rtm_flags = RTM_F_NOTIFY
	);
}

static void
print_rtmsg(const unsigned int msg_len)
{
	printf("{len=%u, type=RTM_GETROUTE, flags=NLM_F_DUMP"
	       ", seq=0, pid=0}, {rtm_family=AF_UNIX"
	       ", rtm_dst_len=0, rtm_src_len=0"
	       ", rtm_tos=IPTOS_LOWDELAY"
	       ", rtm_table=RT_TABLE_DEFAULT"
	       ", rtm_protocol=RTPROT_KERNEL"
	       ", rtm_scope=RT_SCOPE_UNIVERSE"
	       ", rtm_type=RTN_LOCAL"
	       ", rtm_flags=RTM_F_NOTIFY}",
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
	sprintf(nla_type_str, "%#x /* RTA_??? */", nla_type);
	TEST_NLATTR_(fd, nlh0, hdrlen,
		     init_rtmsg, print_rtmsg,
		     nla_type, nla_type_str,
		     4, pattern, 4,
		     print_quoted_hex(pattern, 4));

	TEST_NLATTR(fd, nlh0, hdrlen,
		    init_rtmsg, print_rtmsg,
		    RTA_DST, 4, pattern, 4,
		    print_quoted_hex(pattern, 4));

	const uint32_t ifindex = ifindex_lo();
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_rtmsg, print_rtmsg,
			   RTA_OIF, pattern, ifindex,
			   printf(IFINDEX_LO_STR));

	const uint32_t rt_class_id = RT_TABLE_DEFAULT;
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_rtmsg, print_rtmsg,
			   RTA_TABLE, pattern, rt_class_id,
			   printf("RT_TABLE_DEFAULT"));

	struct nlattr nla = {
		.nla_type = RTAX_LOCK,
		.nla_len = sizeof(nla)
	};
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_rtmsg, print_rtmsg,
			   RTA_METRICS, pattern, nla,
			   printf("{nla_len=%u, nla_type=RTAX_LOCK}",
				  nla.nla_len));
	struct rtnexthop nh = {
		.rtnh_len = sizeof(nh) - 1,
		.rtnh_flags = RTNH_F_DEAD,
		.rtnh_hops = 0xab,
		.rtnh_ifindex = ifindex_lo()
	};
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_rtmsg, print_rtmsg,
			   RTA_MULTIPATH, pattern, nh,
			   printf("{rtnh_len=%u, rtnh_flags=RTNH_F_DEAD"
				  ", rtnh_hops=%u"
				  ", rtnh_ifindex=" IFINDEX_LO_STR "}",
				  nh.rtnh_len, nh.rtnh_hops));

	char buf[RTNH_ALIGN(sizeof(nh)) + sizeof(nla)];
	nh.rtnh_len = sizeof(buf);
	nla.nla_type = RTA_DST;
	memcpy(buf, &nh, sizeof(nh));
	memcpy(buf + RTNH_ALIGN(sizeof(nh)), &nla, sizeof(nla));
	TEST_NLATTR(fd, nlh0, hdrlen,
		    init_rtmsg, print_rtmsg,
		    RTA_MULTIPATH, sizeof(buf), buf, sizeof(buf),
		    printf("{rtnh_len=%u, rtnh_flags=RTNH_F_DEAD"
			   ", rtnh_hops=%u, rtnh_ifindex=" IFINDEX_LO_STR "}"
			   ", {nla_len=%u, nla_type=RTA_DST}",
			   nh.rtnh_len, nh.rtnh_hops, nla.nla_len));

	static const struct rta_cacheinfo ci = {
		.rta_clntref = 0xabcdefab,
		.rta_lastuse = 0xbdadaedc,
		.rta_expires = 0xcdadebad,
		.rta_error = 0xdaedadeb,
		.rta_used = 0xedfabdad,
		.rta_id = 0xfeadbcda,
		.rta_ts = 0xacdbaded,
		.rta_tsage = 0xbadeadef
	};
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_rtmsg, print_rtmsg,
			   RTA_CACHEINFO, pattern, ci,
			   PRINT_FIELD_U("{", ci, rta_clntref);
			   PRINT_FIELD_U(", ", ci, rta_lastuse);
			   PRINT_FIELD_U(", ", ci, rta_expires);
			   PRINT_FIELD_U(", ", ci, rta_error);
			   PRINT_FIELD_U(", ", ci, rta_used);
			   PRINT_FIELD_X(", ", ci, rta_id);
			   PRINT_FIELD_U(", ", ci, rta_ts);
			   PRINT_FIELD_U(", ", ci, rta_tsage);
			   printf("}"));

#ifdef HAVE_STRUCT_RTA_MFC_STATS
	static const struct rta_mfc_stats mfcs = {
		.mfcs_packets = 0xadcdedfdadefadcd,
		.mfcs_bytes = 0xbaedadedcdedadbd,
		.mfcs_wrong_if = 0xcddeabeedaedabfa
	};
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_rtmsg, print_rtmsg,
			   RTA_MFC_STATS, pattern, mfcs,
			   PRINT_FIELD_U("{", mfcs, mfcs_packets);
			   PRINT_FIELD_U(", ", mfcs, mfcs_bytes);
			   PRINT_FIELD_U(", ", mfcs, mfcs_wrong_if);
			   printf("}"));
#endif

#ifdef HAVE_STRUCT_RTVIA
	static const struct rtvia via = {
		.rtvia_family = AF_INET
	};

	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_rtmsg, print_rtmsg,
			   RTA_VIA, pattern, via,
			   printf("{rtvia_family=AF_INET}"));

	static const char address4[] = "12.34.56.78";
	struct in_addr a4 = {
		.s_addr = inet_addr(address4)
	};
	char rtviabuf[sizeof(via) + sizeof(a4)];
	memcpy(rtviabuf, &via, sizeof(via));
	memcpy(rtviabuf + sizeof(via), &a4, sizeof(a4));

	TEST_NLATTR(fd, nlh0, hdrlen,
		    init_rtmsg, print_rtmsg,
		    RTA_VIA, sizeof(rtviabuf), rtviabuf, sizeof(rtviabuf),
		    printf("{rtvia_family=AF_INET"
			   ", rtvia_addr=inet_addr(\"%s\")}", address4));
#endif

	const uint16_t encap_type = LWTUNNEL_ENCAP_NONE;
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_rtmsg, print_rtmsg,
			   RTA_ENCAP_TYPE, pattern, encap_type,
			   printf("LWTUNNEL_ENCAP_NONE"));

	puts("+++ exited with 0 +++");
	return 0;
}
