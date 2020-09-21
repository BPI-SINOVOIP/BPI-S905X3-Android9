/*
 * Copyright (c) 2014-2016 Oracle and/or its affiliates. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alexey Kodanev <alexey.kodanev@oracle.com>
 *
 */

#include <pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <linux/dccp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "lapi/dccp.h"
#include "lapi/netinet_in.h"
#include "lapi/posix_clocks.h"
#include "lapi/socket.h"
#include "lapi/tcp.h"
#include "tst_safe_stdio.h"
#include "tst_safe_pthread.h"
#include "tst_test.h"

static const int max_msg_len = (1 << 16) - 1;
static const int min_msg_len = 5;

enum {
	SERVER_HOST = 0,
	CLIENT_HOST,
};
static char *client_mode;

enum {
	TFO_DISABLED = 0,
	TFO_ENABLED,
};
static int tfo_value = -1;
static char *fastopen_api, *fastopen_sapi;

static const char tfo_cfg[]		= "/proc/sys/net/ipv4/tcp_fastopen";
static const char tcp_tw_reuse[]	= "/proc/sys/net/ipv4/tcp_tw_reuse";
static int tw_reuse_changed;
static int tfo_cfg_value;
static int tfo_cfg_changed;
static int tfo_queue_size	= 100;
static int max_queue_len	= 100;
static const int client_byte	= 0x43;
static const int server_byte	= 0x53;
static const int start_byte	= 0x24;
static const int start_fin_byte	= 0x25;
static const int end_byte	= 0x0a;
static int init_cln_msg_len	= 32;
static int init_srv_msg_len	= 128;
static int max_rand_msg_len;

/*
 * The number of requests from client after
 * which server has to close the connection.
 */
static int server_max_requests	= 3;
static int client_max_requests	= 10;
static int clients_num;
static char *tcp_port		= "61000";
static char *server_addr	= "localhost";
static char *source_addr;
static int busy_poll		= -1;
static int max_etime_cnt = 12; /* ~30 sec max timeout if no connection */

enum {
	TYPE_TCP = 0,
	TYPE_UDP,
	TYPE_DCCP,
	TYPE_SCTP
};
static uint proto_type;
static char *type;
static int sock_type = SOCK_STREAM;
static int protocol;
static int family = AF_INET6;

static uint32_t service_code = 0xffff;

/* server socket */
static int sfd;

/* how long a client must wait for the server's reply */
static int wait_timeout = 60000;

/* in the end test will save time result in this file */
static char *rpath = "tfo_result";

static char *narg, *Narg, *qarg, *rarg, *Rarg, *aarg, *Targ, *barg, *targ,
	    *Aarg;

/* common structure for TCP/UDP server and TCP/UDP client */
struct net_func {
	void (*init)(void);
	void (*run)(void);
	void (*cleanup)(void);
};
static struct net_func net;

#define MAX_THREADS	10000
static pthread_attr_t attr;
static pthread_t *thread_ids;

static struct addrinfo *remote_addrinfo;
static struct addrinfo *local_addrinfo;
static struct sockaddr_storage remote_addr;
static socklen_t remote_addr_len;

static void init_socket_opts(int sd)
{
	if (busy_poll >= 0)
		SAFE_SETSOCKOPT_INT(sd, SOL_SOCKET, SO_BUSY_POLL, busy_poll);

	if (proto_type == TYPE_DCCP) {
		SAFE_SETSOCKOPT_INT(sd, SOL_DCCP, DCCP_SOCKOPT_SERVICE,
			service_code);
	}

	if (client_mode && fastopen_sapi)
		SAFE_SETSOCKOPT_INT(sd, IPPROTO_TCP, TCP_FASTOPEN_CONNECT, 1);
}

static void do_cleanup(void)
{
	if (net.cleanup)
		net.cleanup();

	if (tfo_cfg_changed) {
		tst_res(TINFO, "unset '%s' back to '%d'",
			tfo_cfg, tfo_cfg_value);
		SAFE_FILE_PRINTF(tfo_cfg, "%d", tfo_cfg_value);
	}

	if (tw_reuse_changed) {
		SAFE_FILE_PRINTF(tcp_tw_reuse, "0");
		tst_res(TINFO, "unset '%s' back to '0'", tcp_tw_reuse);
	}
}
TST_DECLARE_ONCE_FN(cleanup, do_cleanup)

static int sock_recv_poll(int fd, char *buf, int buf_size, int offset,
			  int *timeout)
{
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	int len = -1;

	while (1) {
		errno = 0;
		int ret = poll(&pfd, 1, *timeout);
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			break;
		}

		if (ret == 0) {
			errno = ETIME;
			break;
		}

		if (ret != 1 || !(pfd.revents & POLLIN))
			break;

		errno = 0;
		len = recvfrom(fd, buf + offset, buf_size - offset,
			       MSG_DONTWAIT, (struct sockaddr *)&remote_addr,
			       &remote_addr_len);

		if (len == -1 && errno == EINTR)
			continue;

		if (len == 0)
			errno = ESHUTDOWN;

		break;
	}

	return len;
}

static int client_recv(int *fd, char *buf, int srv_msg_len, int *etime_cnt,
		       int *timeout)
{
	int len, offset = 0;

	while (1) {
		errno = 0;
		len = sock_recv_poll(*fd, buf, srv_msg_len, offset, timeout);

		/* socket closed or msg is not valid */
		if (len < 1 || (offset + len) > srv_msg_len ||
		   (buf[0] != start_byte && buf[0] != start_fin_byte)) {
			if (!errno)
				errno = ENOMSG;
			break;
		}
		offset += len;
		if (buf[offset - 1] != end_byte)
			continue;

		/* recv last msg, close socket */
		if (buf[0] == start_fin_byte)
			break;
		return 0;
	}

	if (errno == ETIME && sock_type != SOCK_STREAM) {
		if (++(*etime_cnt) > max_etime_cnt)
			tst_brk(TFAIL, "protocol timeout: %dms", *timeout);
		/* Increase timeout in poll up to 3.2 sec */
		if (*timeout < 3000)
			*timeout <<= 1;
		return 0;
	}

	SAFE_CLOSE(*fd);
	return (errno) ? -1 : 0;
}

static int client_connect_send(const char *msg, int size)
{
	int cfd = SAFE_SOCKET(family, sock_type, protocol);

	init_socket_opts(cfd);

	if (fastopen_api) {
		/* Replaces connect() + send()/write() */
		SAFE_SENDTO(1, cfd, msg, size, MSG_FASTOPEN | MSG_NOSIGNAL,
			remote_addrinfo->ai_addr, remote_addrinfo->ai_addrlen);
	} else {
		if (local_addrinfo)
			SAFE_BIND(cfd, local_addrinfo->ai_addr,
				  local_addrinfo->ai_addrlen);
		/* old TCP API */
		SAFE_CONNECT(cfd, remote_addrinfo->ai_addr,
			     remote_addrinfo->ai_addrlen);
		SAFE_SEND(1, cfd, msg, size, MSG_NOSIGNAL);
	}

	return cfd;
}

union net_size_field {
	char bytes[2];
	uint16_t value;
};

static void make_client_request(char client_msg[], int *cln_len, int *srv_len)
{
	if (max_rand_msg_len)
		*cln_len = *srv_len = min_msg_len + rand() % max_rand_msg_len;

	memset(client_msg, client_byte, *cln_len);
	client_msg[0] = start_byte;

	/* set size for reply */
	union net_size_field net_size;

	net_size.value = htons(*srv_len);
	client_msg[1] = net_size.bytes[0];
	client_msg[2] = net_size.bytes[1];

	client_msg[*cln_len - 1] = end_byte;
}

void *client_fn(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int cln_len = init_cln_msg_len,
	    srv_len = init_srv_msg_len;
	char buf[max_msg_len];
	char client_msg[max_msg_len];
	int cfd, i = 0, etime_cnt = 0;
	intptr_t err = 0;
	int timeout = wait_timeout;

	make_client_request(client_msg, &cln_len, &srv_len);

	/* connect & send requests */
	cfd = client_connect_send(client_msg, cln_len);
	if (cfd == -1) {
		err = errno;
		goto out;
	}

	if (client_recv(&cfd, buf, srv_len, &etime_cnt, &timeout)) {
		err = errno;
		goto out;
	}

	for (i = 1; i < client_max_requests; ++i) {
		if (proto_type == TYPE_UDP)
			goto send;

		if (cfd == -1) {
			cfd = client_connect_send(client_msg, cln_len);
			if (cfd == -1) {
				err = errno;
				goto out;
			}

			if (client_recv(&cfd, buf, srv_len, &etime_cnt,
			    &timeout)) {
				err = errno;
				break;
			}
			continue;
		}

send:
		if (max_rand_msg_len)
			make_client_request(client_msg, &cln_len, &srv_len);

		SAFE_SEND(1, cfd, client_msg, cln_len, MSG_NOSIGNAL);

		if (client_recv(&cfd, buf, srv_len, &etime_cnt, &timeout)) {
			err = errno;
			break;
		}
	}

	if (cfd != -1)
		SAFE_CLOSE(cfd);

out:
	if (i != client_max_requests)
		tst_res(TWARN, "client exit on '%d' request", i);

	return (void *) err;
}

static int parse_client_request(const char *msg)
{
	union net_size_field net_size;
	net_size.bytes[0] = msg[1];
	net_size.bytes[1] = msg[2];
	int size = ntohs(net_size.value);
	if (size < 2 || size > max_msg_len)
		return -1;

	return size;
}

static struct timespec tv_client_start;
static struct timespec tv_client_end;

static void setup_addrinfo(const char *src_addr, const char *port,
			   const struct addrinfo *hints,
			   struct addrinfo **addr_info)
{
	int err = getaddrinfo(src_addr, port, hints, addr_info);

	if (err)
		tst_brk(TBROK, "getaddrinfo failed, %s", gai_strerror(err));

	if (!*addr_info)
		tst_brk(TBROK, "failed to get the address");
}

static void client_init(void)
{
	if (clients_num >= MAX_THREADS) {
		tst_brk(TBROK, "Unexpected num of clients '%d'",
			clients_num);
	}

	thread_ids = SAFE_MALLOC(sizeof(pthread_t) * clients_num);

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = sock_type;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	if (source_addr)
		setup_addrinfo(source_addr, NULL, &hints, &local_addrinfo);
	setup_addrinfo(server_addr, tcp_port, &hints, &remote_addrinfo);

	tst_res(TINFO, "Running the test over IPv%s",
		(remote_addrinfo->ai_family == AF_INET6) ? "6" : "4");

	family = remote_addrinfo->ai_family;

	clock_gettime(CLOCK_MONOTONIC_RAW, &tv_client_start);
	int i;
	for (i = 0; i < clients_num; ++i)
		SAFE_PTHREAD_CREATE(&thread_ids[i], 0, client_fn, NULL);
}

static void client_run(void)
{
	void *res = NULL;
	long clnt_time = 0;
	int i;
	for (i = 0; i < clients_num; ++i) {
		pthread_join(thread_ids[i], &res);
		if (res) {
			tst_brk(TBROK, "client[%d] failed: %s",
				i, strerror((intptr_t)res));
		}
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &tv_client_end);
	clnt_time = (tv_client_end.tv_sec - tv_client_start.tv_sec) * 1000 +
		(tv_client_end.tv_nsec - tv_client_start.tv_nsec) / 1000000;

	tst_res(TINFO, "total time '%ld' ms", clnt_time);

	char client_msg[min_msg_len];
	int msg_len = min_msg_len;

	max_rand_msg_len = 0;
	make_client_request(client_msg, &msg_len, &msg_len);
	/* ask server to terminate */
	client_msg[0] = start_fin_byte;
	int cfd = client_connect_send(client_msg, msg_len);
	if (cfd != -1) {
		shutdown(cfd, SHUT_WR);
		SAFE_CLOSE(cfd);
	}
	/* the script tcp_fastopen_run.sh will remove it */
	SAFE_FILE_PRINTF(rpath, "%ld", clnt_time);

	tst_res(TPASS, "test completed");
}

static void client_cleanup(void)
{
	free(thread_ids);

	if (remote_addrinfo)
		freeaddrinfo(remote_addrinfo);
}

static void make_server_reply(char *send_msg, int size)
{
	memset(send_msg, server_byte, size - 1);
	send_msg[0] = start_byte;
	send_msg[size - 1] = end_byte;
}

void *server_fn(void *cfd)
{
	int client_fd = (intptr_t) cfd;
	int num_requests = 0, offset = 0;
	int timeout = wait_timeout;
	/* Reply will be constructed from first client request */
	char send_msg[max_msg_len];
	int send_msg_len = 0;
	char recv_msg[max_msg_len];
	ssize_t recv_len;

	send_msg[0] = '\0';

	init_socket_opts(client_fd);

	while (1) {
		recv_len = sock_recv_poll(client_fd, recv_msg,
			max_msg_len, offset, &timeout);

		if (recv_len == 0)
			break;

		if (recv_len < 0 || (offset + recv_len) > max_msg_len ||
		   (recv_msg[0] != start_byte &&
		    recv_msg[0] != start_fin_byte)) {
			tst_res(TFAIL, "recv failed, sock '%d'", client_fd);
			goto out;
		}

		offset += recv_len;

		if (recv_msg[offset - 1] != end_byte) {
			/* msg is not complete, continue recv */
			continue;
		}

		/* client asks to terminate */
		if (recv_msg[0] == start_fin_byte)
			goto out;

		send_msg_len = parse_client_request(recv_msg);
		if (send_msg_len < 0) {
			tst_res(TFAIL, "wrong msg size '%d'",
				send_msg_len);
			goto out;
		}
		make_server_reply(send_msg, send_msg_len);

		offset = 0;

		/*
		 * It will tell client that server is going
		 * to close this connection.
		 */
		if (sock_type == SOCK_STREAM &&
		    ++num_requests >= server_max_requests)
			send_msg[0] = start_fin_byte;

		switch (proto_type) {
		case TYPE_SCTP:
			SAFE_SEND(1, client_fd, send_msg, send_msg_len,
				MSG_NOSIGNAL);
		break;
		default:
			SAFE_SENDTO(1, client_fd, send_msg, send_msg_len,
				MSG_NOSIGNAL, (struct sockaddr *)&remote_addr,
				remote_addr_len);
		}

		if (sock_type == SOCK_STREAM &&
		    num_requests >= server_max_requests) {
			/* max reqs, close socket */
			shutdown(client_fd, SHUT_WR);
			break;
		}
	}

	SAFE_CLOSE(client_fd);
	return NULL;

out:
	SAFE_CLOSE(client_fd);
	tst_brk(TBROK, "Server closed");
	return NULL;
}

static pthread_t server_thread_add(intptr_t client_fd)
{
	pthread_t id;
	SAFE_PTHREAD_CREATE(&id, &attr, server_fn, (void *) client_fd);
	return id;
}

static void server_init(void)
{
	char *src_addr = NULL;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = sock_type;
	hints.ai_flags = AI_PASSIVE;

	if (source_addr && !strchr(source_addr, ':'))
		SAFE_ASPRINTF(&src_addr, "::ffff:%s", source_addr);
	setup_addrinfo(src_addr ? src_addr : source_addr, tcp_port,
		       &hints, &local_addrinfo);
	free(src_addr);

	/* IPv6 socket is also able to access IPv4 protocol stack */
	sfd = SAFE_SOCKET(family, sock_type, protocol);
	SAFE_SETSOCKOPT_INT(sfd, SOL_SOCKET, SO_REUSEADDR, 1);

	tst_res(TINFO, "assigning a name to the server socket...");
	SAFE_BIND(sfd, local_addrinfo->ai_addr, local_addrinfo->ai_addrlen);

	freeaddrinfo(local_addrinfo);

	if (proto_type == TYPE_UDP)
		return;

	init_socket_opts(sfd);

	if (fastopen_api || fastopen_sapi) {
		SAFE_SETSOCKOPT_INT(sfd, IPPROTO_TCP, TCP_FASTOPEN,
			tfo_queue_size);
	}

	SAFE_LISTEN(sfd, max_queue_len);
	tst_res(TINFO, "Listen on the socket '%d', port '%s'", sfd, tcp_port);
}

static void server_cleanup(void)
{
	SAFE_CLOSE(sfd);
}

static void server_run_udp(void)
{
	pthread_t p_id = server_thread_add(sfd);

	SAFE_PTHREAD_JOIN(p_id, NULL);
}

static void server_run(void)
{
	/* IPv4 source address will be mapped to IPv6 address */
	struct sockaddr_in6 addr6;
	socklen_t addr_size = sizeof(addr6);

	pthread_attr_init(&attr);

	/*
	 * detaching threads allow to reclaim thread's resources
	 * once a thread finishes its work.
	 */
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
		tst_brk(TBROK | TERRNO, "setdetachstate failed");

	while (1) {
		int client_fd = accept(sfd, (struct sockaddr *)&addr6,
			&addr_size);

		if (client_fd == -1)
			tst_brk(TBROK, "Can't create client socket");

		server_thread_add(client_fd);
	}
}

static void require_root(const char *file)
{
	if (!geteuid())
		return;
	tst_brk(TCONF, "Test needs to be run as root to change %s", file);
}

static void check_tfo_value(void)
{
	/* Check if we can write to tcp_fastopen knob. We might be
	 * inside netns and either have read-only permission or
	 * doesn't have the knob at all.
	 */
	if (access(tfo_cfg, W_OK) < 0) {
		/* TODO check /proc/self/ns/ or TST_USE_NETNS env var */
		tst_res(TINFO, "can't read %s, assume server runs in netns",
			tfo_cfg);
		return;
	}

	SAFE_FILE_SCANF(tfo_cfg, "%d", &tfo_cfg_value);
	tst_res(TINFO, "'%s' is %d", tfo_cfg, tfo_cfg_value);

	/* The check can be the first in this function but set here
	 * to allow to print information about the currently set config
	 */
	if (tfo_value < 0)
		return;

	if (tfo_cfg_value == tfo_value)
		return;

	require_root(tfo_cfg);

	tst_res(TINFO, "set '%s' to '%d'", tfo_cfg, tfo_value);

	SAFE_FILE_PRINTF(tfo_cfg, "%d", tfo_value);
	tfo_cfg_changed = 1;
}

static void check_tw_reuse(void)
{
	if (access(tcp_tw_reuse, W_OK) < 0)
		return;

	int reuse_value = 0;

	SAFE_FILE_SCANF(tcp_tw_reuse, "%d", &reuse_value);
	if (reuse_value) {
		tst_res(TINFO, "tcp_tw_reuse is already set");
		return;
	}

	require_root(tfo_cfg);

	SAFE_FILE_PRINTF(tcp_tw_reuse, "1");
	tw_reuse_changed = 1;
	tst_res(TINFO, "set '%s' to '1'", tcp_tw_reuse);
}

static void set_protocol_type(void)
{
	if (!type || !strcmp(type, "tcp"))
		proto_type = TYPE_TCP;
	else if (!strcmp(type, "udp"))
		proto_type = TYPE_UDP;
	else if (!strcmp(type, "dccp"))
		proto_type = TYPE_DCCP;
	else if (!strcmp(type, "sctp"))
		proto_type = TYPE_SCTP;
}

static void setup(void)
{
	if (tst_parse_int(aarg, &clients_num, 1, INT_MAX))
		tst_brk(TBROK, "Invalid client number '%s'", aarg);
	if (tst_parse_int(rarg, &client_max_requests, 1, INT_MAX))
		tst_brk(TBROK, "Invalid client max requests '%s'", rarg);
	if (tst_parse_int(Rarg, &server_max_requests, 1, INT_MAX))
		tst_brk(TBROK, "Invalid server max requests '%s'", Rarg);
	if (tst_parse_int(narg, &init_cln_msg_len, min_msg_len, max_msg_len))
		tst_brk(TBROK, "Invalid client msg size '%s'", narg);
	if (tst_parse_int(Narg, &init_srv_msg_len, min_msg_len, max_msg_len))
		tst_brk(TBROK, "Invalid server msg size '%s'", Narg);
	if (tst_parse_int(qarg, &tfo_queue_size, 1, INT_MAX))
		tst_brk(TBROK, "Invalid TFO queue size '%s'", qarg);
	if (tst_parse_int(Targ, &wait_timeout, 0, INT_MAX))
		tst_brk(TBROK, "Invalid wait timeout '%s'", Targ);
	if (tst_parse_int(barg, &busy_poll, 0, INT_MAX))
		tst_brk(TBROK, "Invalid busy poll timeout'%s'", barg);
	if (tst_parse_int(targ, &tfo_value, 0, INT_MAX))
		tst_brk(TBROK, "Invalid net.ipv4.tcp_fastopen '%s'", targ);
	if (tst_parse_int(Aarg, &max_rand_msg_len, 10, max_msg_len))
		tst_brk(TBROK, "Invalid max random payload size '%s'", Aarg);

	if (max_rand_msg_len) {
		max_rand_msg_len -= min_msg_len;
		unsigned int seed = max_rand_msg_len ^ client_max_requests;

		srand(seed);
		tst_res(TINFO, "srand() seed 0x%x", seed);
	}

	/* if client_num is not set, use num of processors */
	if (!clients_num)
		clients_num = sysconf(_SC_NPROCESSORS_ONLN);

	if (tfo_value > 0 && tst_kvercmp(3, 7, 0) < 0)
		tst_brk(TCONF, "Test must be run with kernel 3.7 or newer");

	if (busy_poll >= 0 && tst_kvercmp(3, 11, 0) < 0)
		tst_brk(TCONF, "Test must be run with kernel 3.11 or newer");

	set_protocol_type();

	if (client_mode) {
		tst_res(TINFO, "connection: addr '%s', port '%s'",
			server_addr, tcp_port);
		tst_res(TINFO, "client max req: %d", client_max_requests);
		tst_res(TINFO, "clients num: %d", clients_num);
		if (max_rand_msg_len) {
			tst_res(TINFO, "random msg size [%d %d]",
				min_msg_len, max_rand_msg_len);
		} else {
			tst_res(TINFO, "client msg size: %d", init_cln_msg_len);
			tst_res(TINFO, "server msg size: %d", init_srv_msg_len);
		}
		net.init	= client_init;
		net.run		= client_run;
		net.cleanup	= client_cleanup;

		if (proto_type == TYPE_DCCP || proto_type == TYPE_UDP) {
			tst_res(TINFO, "max timeout errors %d", max_etime_cnt);
			wait_timeout = 100;
		}
		check_tw_reuse();
	} else {
		tst_res(TINFO, "max requests '%d'",
			server_max_requests);
		net.init	= server_init;
		switch (proto_type) {
		case TYPE_TCP:
		case TYPE_DCCP:
		case TYPE_SCTP:
			net.run		= server_run;
			net.cleanup	= server_cleanup;
		break;
		case TYPE_UDP:
			net.run		= server_run_udp;
			net.cleanup	= NULL;
		break;
		}
	}

	remote_addr_len = sizeof(struct sockaddr_storage);

	switch (proto_type) {
	case TYPE_TCP:
		tst_res(TINFO, "TCP %s is using %s TCP API.",
			(client_mode) ? "client" : "server",
			(fastopen_api) ? "Fastopen" : "old");
		check_tfo_value();
	break;
	case TYPE_UDP:
		tst_res(TINFO, "using UDP");
		fastopen_api = fastopen_sapi = NULL;
		sock_type = SOCK_DGRAM;
	break;
	case TYPE_DCCP:
		tst_res(TINFO, "DCCP %s", (client_mode) ? "client" : "server");
		fastopen_api = fastopen_sapi = NULL;
		sock_type = SOCK_DCCP;
		protocol = IPPROTO_DCCP;
		service_code = htonl(service_code);
	break;
	case TYPE_SCTP:
		tst_res(TINFO, "SCTP %s", (client_mode) ? "client" : "server");
		fastopen_api = fastopen_sapi = NULL;
		protocol = IPPROTO_SCTP;
	break;
	}

	net.init();
}

static void do_test(void)
{
	net.run();
}

static struct tst_option options[] = {
	{"f", &fastopen_api, "-f       Use TFO API, default is old API"},
	{"F", &fastopen_sapi,
		"-F       TCP_FASTOPEN_CONNECT socket option and standard API"},
	{"t:", &targ, "-t x     Set tcp_fastopen value"},

	{"S:", &source_addr, "-S x     Source address to bind"},
	{"g:", &tcp_port, "-g x     x - server port"},
	{"b:", &barg, "-b x     x - low latency busy poll timeout"},
	{"T:", &type, "-T x     tcp (default), udp, dccp, sctp\n"},

	{"H:", &server_addr, "Client:\n-H x     Server name or IP address"},
	{"l", &client_mode, "-l       Become client, default is server"},
	{"a:", &aarg, "-a x     Number of clients running in parallel"},
	{"r:", &rarg, "-r x     Number of client requests"},
	{"n:", &narg, "-n x     Client message size"},
	{"N:", &Narg, "-N x     Server message size"},
	{"m:", &Targ, "-m x     Reply timeout in microsec."},
	{"d:", &rpath, "-d x     x is a path to file where result is saved"},
	{"A:", &Aarg, "-A x     x max payload length (generated randomly)\n"},

	{"R:", &Rarg, "Server:\n-R x     x requests after which conn.closed"},
	{"q:", &qarg, "-q x     x - TFO queue"},
	{NULL, NULL, NULL}
};

static struct tst_test test = {
	.test_all = do_test,
	.setup = setup,
	.cleanup = cleanup,
	.options = options
};
