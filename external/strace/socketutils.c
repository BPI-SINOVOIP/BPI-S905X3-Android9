/*
 * Copyright (c) 2014 Zubin Mithra <zubin.mithra@gmail.com>
 * Copyright (c) 2014-2016 Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (c) 2014-2018 The strace developers.
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
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "netlink.h"
#include <linux/sock_diag.h>
#include <linux/inet_diag.h>
#include <linux/unix_diag.h>
#include <linux/netlink_diag.h>
#include <linux/rtnetlink.h>
#if HAVE_LINUX_GENETLINK_H
#include <linux/genetlink.h>
#endif

#include <sys/un.h>
#ifndef UNIX_PATH_MAX
# define UNIX_PATH_MAX sizeof(((struct sockaddr_un *) 0)->sun_path)
#endif

#include "xstring.h"

typedef struct {
	unsigned long inode;
	char *details;
} cache_entry;

#define CACHE_SIZE 1024U
static cache_entry cache[CACHE_SIZE];
#define CACHE_MASK (CACHE_SIZE - 1)

static int
cache_inode_details(const unsigned long inode, char *const details)
{
	cache_entry *e = &cache[inode & CACHE_MASK];
	free(e->details);
	e->inode = inode;
	e->details = details;

	return 1;
}

static const char *
get_sockaddr_by_inode_cached(const unsigned long inode)
{
	const cache_entry *const e = &cache[inode & CACHE_MASK];
	return (e && inode == e->inode) ? e->details : NULL;
}

static bool
print_sockaddr_by_inode_cached(const unsigned long inode)
{
	const char *const details = get_sockaddr_by_inode_cached(inode);
	if (details) {
		tprints(details);
		return true;
	}
	return false;
}

static bool
send_query(struct tcb *tcp, const int fd, void *req, size_t req_size)
{
	struct sockaddr_nl nladdr = {
		.nl_family = AF_NETLINK
	};
	struct iovec iov = {
		.iov_base = req,
		.iov_len = req_size
	};
	const struct msghdr msg = {
		.msg_name = &nladdr,
		.msg_namelen = sizeof(nladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1
	};

	for (;;) {
		if (sendmsg(fd, &msg, 0) < 0) {
			if (errno == EINTR)
				continue;
			return false;
		}
		return true;
	}
}

static bool
inet_send_query(struct tcb *tcp, const int fd, const int family,
		const int proto)
{
	struct {
		const struct nlmsghdr nlh;
		const struct inet_diag_req_v2 idr;
	} req = {
		.nlh = {
			.nlmsg_len = sizeof(req),
			.nlmsg_type = SOCK_DIAG_BY_FAMILY,
			.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST
		},
		.idr = {
			.sdiag_family = family,
			.sdiag_protocol = proto,
			.idiag_states = -1
		}
	};
	return send_query(tcp, fd, &req, sizeof(req));
}

static int
inet_parse_response(const void *const data, const int data_len,
		    const unsigned long inode, void *opaque_data)
{
	const char *const proto_name = opaque_data;
	const struct inet_diag_msg *const diag_msg = data;
	static const char zero_addr[sizeof(struct in6_addr)];
	socklen_t addr_size, text_size;

	if (data_len < (int) NLMSG_LENGTH(sizeof(*diag_msg)))
		return -1;
	if (diag_msg->idiag_inode != inode)
		return 0;

	switch (diag_msg->idiag_family) {
		case AF_INET:
			addr_size = sizeof(struct in_addr);
			text_size = INET_ADDRSTRLEN;
			break;
		case AF_INET6:
			addr_size = sizeof(struct in6_addr);
			text_size = INET6_ADDRSTRLEN;
			break;
		default:
			return -1;
	}

	char src_buf[text_size];
	char *details;

	if (!inet_ntop(diag_msg->idiag_family, diag_msg->id.idiag_src,
		       src_buf, text_size))
		return -1;

	if (diag_msg->id.idiag_dport ||
	    memcmp(zero_addr, diag_msg->id.idiag_dst, addr_size)) {
		char dst_buf[text_size];

		if (!inet_ntop(diag_msg->idiag_family, diag_msg->id.idiag_dst,
			       dst_buf, text_size))
			return -1;

		if (asprintf(&details, "%s:[%s:%u->%s:%u]", proto_name,
			     src_buf, ntohs(diag_msg->id.idiag_sport),
			     dst_buf, ntohs(diag_msg->id.idiag_dport)) < 0)
			return false;
	} else {
		if (asprintf(&details, "%s:[%s:%u]", proto_name, src_buf,
			     ntohs(diag_msg->id.idiag_sport)) < 0)
			return false;
	}

	return cache_inode_details(inode, details);
}

static bool
receive_responses(struct tcb *tcp, const int fd, const unsigned long inode,
		  const unsigned long expected_msg_type,
		  int (*parser)(const void *, int,
				unsigned long, void *),
		  void *opaque_data)
{
	static union {
		struct nlmsghdr hdr;
		long buf[8192 / sizeof(long)];
	} hdr_buf;

	struct sockaddr_nl nladdr = {
		.nl_family = AF_NETLINK
	};
	struct iovec iov = {
		.iov_base = hdr_buf.buf,
		.iov_len = sizeof(hdr_buf.buf)
	};
	int flags = 0;

	for (;;) {
		struct msghdr msg = {
			.msg_name = &nladdr,
			.msg_namelen = sizeof(nladdr),
			.msg_iov = &iov,
			.msg_iovlen = 1
		};

		ssize_t ret = recvmsg(fd, &msg, flags);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return false;
		}

		const struct nlmsghdr *h = &hdr_buf.hdr;
		if (!NLMSG_OK(h, ret))
			return false;
		for (; NLMSG_OK(h, ret); h = NLMSG_NEXT(h, ret)) {
			if (h->nlmsg_type != expected_msg_type)
				return false;
			const int rc = parser(NLMSG_DATA(h),
					      h->nlmsg_len, inode, opaque_data);
			if (rc > 0)
				return true;
			if (rc < 0)
				return false;
		}
		flags = MSG_DONTWAIT;
	}
}

static bool
unix_send_query(struct tcb *tcp, const int fd, const unsigned long inode)
{
	struct {
		const struct nlmsghdr nlh;
		const struct unix_diag_req udr;
	} req = {
		.nlh = {
			.nlmsg_len = sizeof(req),
			.nlmsg_type = SOCK_DIAG_BY_FAMILY,
			.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST
		},
		.udr = {
			.sdiag_family = AF_UNIX,
			.udiag_ino = inode,
			.udiag_states = -1,
			.udiag_show = UDIAG_SHOW_NAME | UDIAG_SHOW_PEER
		}
	};
	return send_query(tcp, fd, &req, sizeof(req));
}

static int
unix_parse_response(const void *data, const int data_len,
		    const unsigned long inode, void *opaque_data)
{
	const char *proto_name = opaque_data;
	const struct unix_diag_msg *diag_msg = data;
	struct rtattr *attr;
	int rta_len = data_len - NLMSG_LENGTH(sizeof(*diag_msg));
	uint32_t peer = 0;
	size_t path_len = 0;
	char path[UNIX_PATH_MAX + 1];

	if (rta_len < 0)
		return -1;
	if (diag_msg->udiag_ino != inode)
		return 0;
	if (diag_msg->udiag_family != AF_UNIX)
		return -1;

	for (attr = (struct rtattr *) (diag_msg + 1);
	     RTA_OK(attr, rta_len);
	     attr = RTA_NEXT(attr, rta_len)) {
		switch (attr->rta_type) {
		case UNIX_DIAG_NAME:
			if (!path_len) {
				path_len = RTA_PAYLOAD(attr);
				if (path_len > UNIX_PATH_MAX)
					path_len = UNIX_PATH_MAX;
				memcpy(path, RTA_DATA(attr), path_len);
				path[path_len] = '\0';
			}
			break;
		case UNIX_DIAG_PEER:
			if (RTA_PAYLOAD(attr) >= 4)
				peer = *(uint32_t *) RTA_DATA(attr);
			break;
		}
	}

	/*
	 * print obtained information in the following format:
	 * "UNIX:[" SELF_INODE [ "->" PEER_INODE ][ "," SOCKET_FILE ] "]"
	 */
	if (!peer && !path_len)
		return -1;

	char peer_str[3 + sizeof(peer) * 3];
	if (peer)
		xsprintf(peer_str, "->%u", peer);
	else
		peer_str[0] = '\0';

	const char *path_str;
	if (path_len) {
		char *outstr = alloca(4 * path_len + 4);

		outstr[0] = ',';
		if (path[0] == '\0') {
			outstr[1] = '@';
			string_quote(path + 1, outstr + 2,
				     path_len - 1, QUOTE_0_TERMINATED);
		} else {
			string_quote(path, outstr + 1,
				     path_len, QUOTE_0_TERMINATED);
		}
		path_str = outstr;
	} else {
		path_str = "";
	}

	char *details;
	if (asprintf(&details, "%s:[%lu%s%s]", proto_name, inode,
		     peer_str, path_str) < 0)
		return -1;

	return cache_inode_details(inode, details);
}

static bool
netlink_send_query(struct tcb *tcp, const int fd, const unsigned long inode)
{
	struct {
		const struct nlmsghdr nlh;
		const struct netlink_diag_req ndr;
	} req = {
		.nlh = {
			.nlmsg_len = sizeof(req),
			.nlmsg_type = SOCK_DIAG_BY_FAMILY,
			.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST
		},
		.ndr = {
			.sdiag_family = AF_NETLINK,
			.sdiag_protocol = NDIAG_PROTO_ALL,
			.ndiag_show = NDIAG_SHOW_MEMINFO
		}
	};
	return send_query(tcp, fd, &req, sizeof(req));
}

static int
netlink_parse_response(const void *data, const int data_len,
		       const unsigned long inode, void *opaque_data)
{
	const char *proto_name = opaque_data;
	const struct netlink_diag_msg *const diag_msg = data;
	const char *netlink_proto;
	char *details;

	if (data_len < (int) NLMSG_LENGTH(sizeof(*diag_msg)))
		return -1;
	if (diag_msg->ndiag_ino != inode)
		return 0;

	if (diag_msg->ndiag_family != AF_NETLINK)
		return -1;

	netlink_proto = xlookup(netlink_protocols,
				diag_msg->ndiag_protocol);

	if (netlink_proto) {
		netlink_proto = STR_STRIP_PREFIX(netlink_proto, "NETLINK_");
		if (asprintf(&details, "%s:[%s:%u]", proto_name,
			     netlink_proto, diag_msg->ndiag_portid) < 0)
			return -1;
	} else {
		if (asprintf(&details, "%s:[%u]", proto_name,
			     (unsigned) diag_msg->ndiag_protocol) < 0)
			return -1;
	}

	return cache_inode_details(inode, details);
}

static const char *
unix_get(struct tcb *tcp, const int fd, const unsigned long inode)
{
	return unix_send_query(tcp, fd, inode)
		&& receive_responses(tcp, fd, inode, SOCK_DIAG_BY_FAMILY,
				     unix_parse_response, (void *) "UNIX")
		? get_sockaddr_by_inode_cached(inode) : NULL;
}

static const char *
inet_get(struct tcb *tcp, const int fd, const int family, const int protocol,
	 const unsigned long inode, const char *proto_name)
{
	return inet_send_query(tcp, fd, family, protocol)
		&& receive_responses(tcp, fd, inode, SOCK_DIAG_BY_FAMILY,
				     inet_parse_response, (void *) proto_name)
		? get_sockaddr_by_inode_cached(inode) : NULL;
}

static const char *
tcp_v4_get(struct tcb *tcp, const int fd, const unsigned long inode)
{
	return inet_get(tcp, fd, AF_INET, IPPROTO_TCP, inode, "TCP");
}

static const char *
udp_v4_get(struct tcb *tcp, const int fd, const unsigned long inode)
{
	return inet_get(tcp, fd, AF_INET, IPPROTO_UDP, inode, "UDP");
}

static const char *
tcp_v6_get(struct tcb *tcp, const int fd, const unsigned long inode)
{
	return inet_get(tcp, fd, AF_INET6, IPPROTO_TCP, inode, "TCPv6");
}

static const char *
udp_v6_get(struct tcb *tcp, const int fd, const unsigned long inode)
{
	return inet_get(tcp, fd, AF_INET6, IPPROTO_UDP, inode, "UDPv6");
}

static const char *
netlink_get(struct tcb *tcp, const int fd, const unsigned long inode)
{
	return netlink_send_query(tcp, fd, inode)
		&& receive_responses(tcp, fd, inode, SOCK_DIAG_BY_FAMILY,
				     netlink_parse_response, (void *) "NETLINK")
		? get_sockaddr_by_inode_cached(inode) : NULL;
}

static const struct {
	const char *const name;
	const char * (*const get)(struct tcb *, int, unsigned long);
} protocols[] = {
	[SOCK_PROTO_UNIX] = { "UNIX", unix_get },
	[SOCK_PROTO_TCP] = { "TCP", tcp_v4_get },
	[SOCK_PROTO_UDP] = { "UDP", udp_v4_get },
	[SOCK_PROTO_TCPv6] = { "TCPv6", tcp_v6_get },
	[SOCK_PROTO_UDPv6] = { "UDPv6", udp_v6_get },
	[SOCK_PROTO_NETLINK] = { "NETLINK", netlink_get }
};

enum sock_proto
get_proto_by_name(const char *const name)
{
	unsigned int i;
	for (i = (unsigned int) SOCK_PROTO_UNKNOWN + 1;
	     i < ARRAY_SIZE(protocols); ++i) {
		if (protocols[i].name && !strcmp(name, protocols[i].name))
			return (enum sock_proto) i;
	}
	return SOCK_PROTO_UNKNOWN;
}

static const char *
get_sockaddr_by_inode_uncached(struct tcb *tcp, const unsigned long inode,
			       const enum sock_proto proto)
{
	if ((unsigned int) proto >= ARRAY_SIZE(protocols) ||
	    (proto != SOCK_PROTO_UNKNOWN && !protocols[proto].get))
		return NULL;

	const int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_SOCK_DIAG);
	if (fd < 0)
		return NULL;
	const char *details = NULL;

	if (proto != SOCK_PROTO_UNKNOWN) {
		details = protocols[proto].get(tcp, fd, inode);
	} else {
		unsigned int i;
		for (i = (unsigned int) SOCK_PROTO_UNKNOWN + 1;
		     i < ARRAY_SIZE(protocols); ++i) {
			if (!protocols[i].get)
				continue;
			details = protocols[i].get(tcp, fd, inode);
			if (details)
				break;
		}
	}

	close(fd);
	return details;
}

static bool
print_sockaddr_by_inode_uncached(struct tcb *tcp, const unsigned long inode,
				 const enum sock_proto proto)
{
	const char *details = get_sockaddr_by_inode_uncached(tcp, inode, proto);

	if (details) {
		tprints(details);
		return true;
	}

	if ((unsigned int) proto < ARRAY_SIZE(protocols) &&
	    protocols[proto].name) {
		tprintf("%s:[%lu]", protocols[proto].name, inode);
		return true;
	}

	return false;
}

/* Given an inode number of a socket, return its protocol details.  */
const char *
get_sockaddr_by_inode(struct tcb *const tcp, const int fd,
		      const unsigned long inode)
{
	const char *details = get_sockaddr_by_inode_cached(inode);
	return details ? details :
		get_sockaddr_by_inode_uncached(tcp, inode, getfdproto(tcp, fd));
}

/* Given an inode number of a socket, print out its protocol details.  */
bool
print_sockaddr_by_inode(struct tcb *const tcp, const int fd,
			const unsigned long inode)
{
	return print_sockaddr_by_inode_cached(inode) ? true :
		print_sockaddr_by_inode_uncached(tcp, inode,
						 getfdproto(tcp, fd));
}

#ifdef HAVE_LINUX_GENETLINK_H
/*
 * Managing the cache for decoding communications of Netlink GENERIC protocol
 *
 * As name shown Netlink GENERIC protocol is generic protocol. The
 * numbers of msg types used in the protocol are not defined
 * statically. Kernel defines them on demand.  So the xlat converted
 * from header files doesn't help for decoding the protocol. Following
 * codes are building xlat(dyxlat) at runtime.
 */
static bool
genl_send_dump_families(struct tcb *tcp, const int fd)
{
	struct {
		const struct nlmsghdr nlh;
		struct genlmsghdr gnlh;
	} req = {
		.nlh = {
			.nlmsg_len = sizeof(req),
			.nlmsg_type = GENL_ID_CTRL,
			.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST,
		},
		.gnlh = {
			.cmd = CTRL_CMD_GETFAMILY,
		}
	};
	return send_query(tcp, fd, &req, sizeof(req));
}

static int
genl_parse_families_response(const void *const data,
			     const int data_len, const unsigned long inode,
			     void *opaque_data)
{
	struct dyxlat *const dyxlat = opaque_data;
	const struct genlmsghdr *const gnlh = data;
	struct rtattr *attr;
	int rta_len = data_len - NLMSG_LENGTH(sizeof(*gnlh));

	char *name = NULL;
	unsigned int name_len = 0;
	uint16_t *id = NULL;

	if (rta_len < 0)
		return -1;
	if (gnlh->cmd != CTRL_CMD_NEWFAMILY)
		return -1;
	if (gnlh->version != 2)
		return -1;

	for (attr = (struct rtattr *) (gnlh + 1);
	     RTA_OK(attr, rta_len);
	     attr = RTA_NEXT(attr, rta_len)) {
		switch (attr->rta_type) {
		case CTRL_ATTR_FAMILY_NAME:
			if (!name) {
				name = RTA_DATA(attr);
				name_len = RTA_PAYLOAD(attr);
			}
			break;
		case CTRL_ATTR_FAMILY_ID:
			if (!id && RTA_PAYLOAD(attr) == sizeof(*id))
				id = RTA_DATA(attr);
			break;
		}

		if (name && id) {
			dyxlat_add_pair(dyxlat, *id, name, name_len);
			name = NULL;
			id = NULL;
		}
	}

	return 0;
}

const struct xlat *
genl_families_xlat(struct tcb *tcp)
{
	static struct dyxlat *dyxlat;

	if (!dyxlat) {
		dyxlat = dyxlat_alloc(32);

		int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
		if (fd < 0)
			goto out;

		if (genl_send_dump_families(tcp, fd))
			receive_responses(tcp, fd, 0, GENL_ID_CTRL,
					  genl_parse_families_response, dyxlat);
		close(fd);
	}

out:
	return dyxlat_get(dyxlat);
}

#else /* !HAVE_LINUX_GENETLINK_H */

const struct xlat *
genl_families_xlat(struct tcb *tcp)
{
	return NULL;
}
#endif
