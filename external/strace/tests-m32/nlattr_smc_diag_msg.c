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
#include <sys/socket.h>

#ifdef AF_SMC

# include <stdio.h>
# include <string.h>
# include <stdint.h>
# include <arpa/inet.h>
# include "test_nlattr.h"
# include <linux/rtnetlink.h>
# include <linux/smc_diag.h>
# include <linux/sock_diag.h>

# ifndef SMC_CLNT
#  define SMC_CLNT 0
# endif
# ifndef SMC_ACTIVE
#  define SMC_ACTIVE 1
# endif

static const char address[] = "12.34.56.78";

static void
init_smc_diag_msg(struct nlmsghdr *const nlh, const unsigned int msg_len)
{
	SET_STRUCT(struct nlmsghdr, nlh,
		.nlmsg_len = msg_len,
		.nlmsg_type = SOCK_DIAG_BY_FAMILY,
		.nlmsg_flags = NLM_F_DUMP
	);

	struct smc_diag_msg *const msg = NLMSG_DATA(nlh);
	SET_STRUCT(struct smc_diag_msg, msg,
		.diag_family = AF_SMC,
		.diag_state = SMC_ACTIVE
	);

	if (!inet_pton(AF_INET, address, msg->id.idiag_src) ||
	    !inet_pton(AF_INET, address, msg->id.idiag_dst))
		perror_msg_and_skip("inet_pton");
}

static void
print_smc_diag_msg(const unsigned int msg_len)
{
	printf("{len=%u, type=SOCK_DIAG_BY_FAMILY"
	       ", flags=NLM_F_DUMP, seq=0, pid=0}"
	       ", {diag_family=AF_SMC, diag_state=SMC_ACTIVE"
	       ", diag_fallback=0, diag_shutdown=0"
	       ", id={idiag_sport=htons(0), idiag_dport=htons(0)"
	       ", idiag_src=inet_addr(\"%s\")"
	       ", idiag_dst=inet_addr(\"%s\")"
	       ", idiag_if=0, idiag_cookie=[0, 0]}"
	       ", diag_uid=0, diag_inode=0}",
	       msg_len, address, address);
}

#define PRINT_FIELD_SMC_DIAG_CURSOR(prefix_, where_, field_)		\
	do {								\
		printf("%s%s=", (prefix_), #field_);			\
		PRINT_FIELD_U("{", (where_).field_, reserved);		\
		PRINT_FIELD_U(", ", (where_).field_, wrap);		\
		PRINT_FIELD_U(", ", (where_).field_, count);		\
		printf("}");						\
	} while (0)

int main(void)
{
	skip_if_unavailable("/proc/self/fd/");

	int fd = create_nl_socket(NETLINK_SOCK_DIAG);
	const unsigned int hdrlen = sizeof(struct smc_diag_msg);
	void *const nlh0 = tail_alloc(NLMSG_SPACE(hdrlen));

	static char pattern[4096];
	fill_memory_ex(pattern, sizeof(pattern), 'a', 'z' - 'a' + 1);

	static const struct smc_diag_conninfo cinfo = {
		.token = 0xabcdefac,
		.sndbuf_size = 0xbcdaefad,
		.rmbe_size = 0xcdbaefab,
		.peer_rmbe_size = 0xdbcdedaf,
		.rx_prod = {
			.reserved = 0xabc1,
			.wrap = 0xbca1,
			.count = 0xcdedbad1
		},
		.rx_cons = {
			.reserved = 0xabc2,
			.wrap = 0xbca2,
			.count = 0xcdedbad2
		},
		.tx_prod = {
			.reserved = 0xabc3,
			.wrap = 0xbca3,
			.count = 0xcdedbad3
		},
		.tx_cons = {
			.reserved = 0xabc4,
			.wrap = 0xbca4,
			.count = 0xcdedbad4
		},
		.rx_prod_flags = 0xff,
		.rx_conn_state_flags = 0xff,
		.tx_prod_flags = 0xff,
		.tx_conn_state_flags = 0xff,
		.tx_prep = {
			.reserved = 0xabc5,
			.wrap = 0xbca5,
			.count = 0xcdedbad5
		},
		.tx_sent = {
			.reserved = 0xabc6,
			.wrap = 0xbca6,
			.count = 0xcdedbad6
		},
		.tx_fin = {
			.reserved = 0xabc7,
			.wrap = 0xbca7,
			.count = 0xcdedbad7
		}
	};

	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_smc_diag_msg, print_smc_diag_msg,
			   SMC_DIAG_CONNINFO, pattern, cinfo,
			   PRINT_FIELD_U("{", cinfo, token);
			   PRINT_FIELD_U(", ", cinfo, sndbuf_size);
			   PRINT_FIELD_U(", ", cinfo, rmbe_size);
			   PRINT_FIELD_U(", ", cinfo, peer_rmbe_size);
			   PRINT_FIELD_SMC_DIAG_CURSOR(", ", cinfo, rx_prod);
			   PRINT_FIELD_SMC_DIAG_CURSOR(", ", cinfo, rx_cons);
			   PRINT_FIELD_SMC_DIAG_CURSOR(", ", cinfo, tx_prod);
			   PRINT_FIELD_SMC_DIAG_CURSOR(", ", cinfo, tx_cons);
			   printf(", rx_prod_flags=0xff");
			   printf(", rx_conn_state_flags=0xff");
			   printf(", tx_prod_flags=0xff");
			   printf(", tx_conn_state_flags=0xff");
			   PRINT_FIELD_SMC_DIAG_CURSOR(", ", cinfo, tx_prep);
			   PRINT_FIELD_SMC_DIAG_CURSOR(", ", cinfo, tx_sent);
			   PRINT_FIELD_SMC_DIAG_CURSOR(", ", cinfo, tx_fin);
			   printf("}"));

	static const struct smc_diag_lgrinfo linfo = {
		.lnk[0] = {
			.link_id = 0xaf,
			.ibport = 0xfa,
			.ibname = "123",
			.gid = "456",
			.peer_gid = "789"
		},
		.role = SMC_CLNT
	};
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_smc_diag_msg, print_smc_diag_msg,
			   SMC_DIAG_LGRINFO, pattern, linfo,
			   PRINT_FIELD_U("{lnk[0]={", linfo.lnk[0], link_id);
			   printf(", ibname=\"%s\"", linfo.lnk[0].ibname);
			   PRINT_FIELD_U(", ", linfo.lnk[0], ibport);
			   printf(", gid=\"%s\"", linfo.lnk[0].gid);
			   printf(", peer_gid=\"%s\"}", linfo.lnk[0].peer_gid);
			   printf(", role=SMC_CLNT}"));

	printf("+++ exited with 0 +++\n");
	return 0;
}

#else

SKIP_MAIN_UNDEFINED("AF_SMC")

#endif
