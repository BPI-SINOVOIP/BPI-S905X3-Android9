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
#include <string.h>
#include <stdint.h>
#include <net/if.h>
#include "test_nlattr.h"
#include <sys/socket.h>
#include <linux/filter.h>
#include <linux/packet_diag.h>
#include <linux/rtnetlink.h>
#include <linux/sock_diag.h>

static void
init_packet_diag_msg(struct nlmsghdr *const nlh, const unsigned int msg_len)
{
	SET_STRUCT(struct nlmsghdr, nlh,
		.nlmsg_len = msg_len,
		.nlmsg_type = SOCK_DIAG_BY_FAMILY,
		.nlmsg_flags = NLM_F_DUMP
	);

	struct packet_diag_msg *const msg = NLMSG_DATA(nlh);
	SET_STRUCT(struct packet_diag_msg, msg,
		.pdiag_family = AF_PACKET,
		.pdiag_type = SOCK_STREAM
	);
}

static void
print_packet_diag_msg(const unsigned int msg_len)
{
	printf("{len=%u, type=SOCK_DIAG_BY_FAMILY"
	       ", flags=NLM_F_DUMP, seq=0, pid=0}"
	       ", {pdiag_family=AF_PACKET"
	       ", pdiag_type=SOCK_STREAM, pdiag_num=0"
	       ", pdiag_ino=0, pdiag_cookie=[0, 0]}",
	       msg_len);
}

static void
print_packet_diag_mclist(const struct packet_diag_mclist *const dml)
{
	printf("{pdmc_index=" IFINDEX_LO_STR);
	PRINT_FIELD_U(", ", *dml, pdmc_count);
	PRINT_FIELD_U(", ", *dml, pdmc_type);
	PRINT_FIELD_U(", ", *dml, pdmc_alen);
	printf(", pdmc_addr=");
	print_quoted_hex(dml->pdmc_addr, dml->pdmc_alen);
	printf("}");
}

static const struct sock_filter filter[] = {
	BPF_STMT(BPF_LD|BPF_B|BPF_ABS, SKF_AD_OFF+SKF_AD_PKTTYPE),
	BPF_STMT(BPF_RET|BPF_K, 0x2a)
};

static void
print_sock_filter(const struct sock_filter *const f)
{
	if (f == filter)
		printf("BPF_STMT(BPF_LD|BPF_B|BPF_ABS"
		       ", SKF_AD_OFF+SKF_AD_PKTTYPE)");
	else
		printf("BPF_STMT(BPF_RET|BPF_K, 0x2a)");
}

int
main(void)
{
	skip_if_unavailable("/proc/self/fd/");

	int fd = create_nl_socket(NETLINK_SOCK_DIAG);
	const unsigned int hdrlen = sizeof(struct packet_diag_msg);
	void *const nlh0 = tail_alloc(NLMSG_SPACE(hdrlen));

	static char pattern[4096];
	fill_memory_ex(pattern, sizeof(pattern), 'a', 'z' - 'a' + 1);

	static const struct packet_diag_info pinfo = {
		.pdi_index = 0xabcddafa,
		.pdi_version = 0xbabcdafb,
		.pdi_reserve = 0xcfaacdaf,
		.pdi_copy_thresh = 0xdabacdaf,
		.pdi_tstamp = 0xeafbaadf,
		.pdi_flags = PDI_RUNNING
	};
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_packet_diag_msg, print_packet_diag_msg,
			   PACKET_DIAG_INFO, pattern, pinfo,
			   PRINT_FIELD_U("{", pinfo, pdi_index);
			   PRINT_FIELD_U(", ", pinfo, pdi_version);
			   PRINT_FIELD_U(", ", pinfo, pdi_reserve);
			   PRINT_FIELD_U(", ", pinfo, pdi_copy_thresh);
			   PRINT_FIELD_U(", ", pinfo, pdi_tstamp);
			   printf(", pdi_flags=PDI_RUNNING}"));

	const struct packet_diag_mclist dml[] = {
		{
			.pdmc_index = ifindex_lo(),
			.pdmc_count = 0xabcdaefc,
			.pdmc_type = 0xcdaf,
			.pdmc_alen = 4,
			.pdmc_addr = "1234"
		},
		{
			.pdmc_index = ifindex_lo(),
			.pdmc_count = 0xdaefeafc,
			.pdmc_type = 0xadef,
			.pdmc_alen = 4,
			.pdmc_addr = "5678"
		}
	};
	TEST_NLATTR_ARRAY(fd, nlh0, hdrlen,
			  init_packet_diag_msg, print_packet_diag_msg,
			  PACKET_DIAG_MCLIST, pattern, dml,
			  print_packet_diag_mclist);

	static const struct packet_diag_ring pdr = {
		.pdr_block_size = 0xabcdafed,
		.pdr_block_nr = 0xbcadefae,
		.pdr_frame_size = 0xcabdfeac,
		.pdr_frame_nr = 0xdeaeadef,
		.pdr_retire_tmo = 0xedbafeac,
		.pdr_sizeof_priv = 0xfeadeacd,
		.pdr_features = 0xadebadea
	};
	TEST_NLATTR_OBJECT(fd, nlh0, hdrlen,
			   init_packet_diag_msg, print_packet_diag_msg,
			   PACKET_DIAG_RX_RING, pattern, pdr,
			   PRINT_FIELD_U("{", pdr, pdr_block_size);
			   PRINT_FIELD_U(", ", pdr, pdr_block_nr);
			   PRINT_FIELD_U(", ", pdr, pdr_frame_size);
			   PRINT_FIELD_U(", ", pdr, pdr_frame_nr);
			   PRINT_FIELD_U(", ", pdr, pdr_retire_tmo);
			   PRINT_FIELD_U(", ", pdr, pdr_sizeof_priv);
			   PRINT_FIELD_U(", ", pdr, pdr_features);
			   printf("}"));

	TEST_NLATTR_ARRAY(fd, nlh0, hdrlen,
			  init_packet_diag_msg, print_packet_diag_msg,
			  PACKET_DIAG_FILTER, pattern, filter,
			  print_sock_filter);

	printf("+++ exited with 0 +++\n");
	return 0;
}
