/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

#ifndef ANDROID_NET_COMMON_H
#define ANDROID_NET_COMMON_H
 #include <inttypes.h>
#include <utils/misc.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <asm/types.h>

/*
The statement
    typedef unsigned short sa_family_t;
is removed from bionic/libc/kernel/common/linux/socket.h.
To avoid compile failure, add the statement here.
*/
typedef unsigned short sa_family_t;

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <net/if_arp.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/rtnetlink.h>  //for ifinfomsg
#include <linux/netlink.h> //for nlmsghdr{}

#define NL_SOCK_INV      -1
#define RET_STR_SZ       4096
#define NL_POLL_MSG_SZ   8*1024
#define SYSFS_PATH_MAX   256

#define SYSFS_CLASS_NET     "/sys/class/net"

typedef int (*pfunc_update_interface_list)(void);
typedef int (*pfunc_is_interested_event)(char *ifname);

typedef struct _interface_info_t {
    int if_index;
    char *name;
} interface_info_t;

#define NR_ETHER_INTERFACES      16
#define NR_PPP_INTERFACES      16

static int open_NETLINK_socket(int netlinkFamily, int groups);
static void parseAsciiNetlinkMessage
(char *buff, int len, char *rbuf, int rbufsize, int *pguard, pfunc_is_interested_event pfunc_check);
static void parseBinaryNetlinkMessage
(char *buff, int len, char *rbuf, int rbufsize, int *pguard, pfunc_is_interested_event pfunc_check);

static int waitForNetInterfaceEvent
(int nl_socket_netlink_route, int nl_socket_kobj_uevent,
char *rbuf, int *pguard, pfunc_update_interface_list pfunc);
static int find_empty_node(interface_info_t *node_table, int nr_nodes);
static int add_interface_node_to_list(interface_info_t *node_table, int nr_nodes, int if_index, char *if_name);
static int getInterfaceName(interface_info_t *node_table, int nr_nodes, int index, char *ifname);

#endif

