/*
 * Copyright 2009, The Android-x86 Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: kejun.gao <greatgauss@gmail.com>
 */

#include "jni.h"
#include "com_droidlogic_common.h"


//namespace android {


/* Same as strlen(x) for constant string literals ONLY */
#define CONST_STRLEN(x)  (sizeof(x)-1)

/* Convenience macro to call has_prefix with a constant string literal  */
#define HAS_CONST_PREFIX(str,end,prefix)  has_prefix((str),(end),prefix,CONST_STRLEN(prefix))



static char flags_desc[512];

static const char * flag_desc_tbl[32] = {
    "UP",
    "BC",
    "DBG",
    "LOOPBACK",

    "PPP",
    "NT",
    "RUNNING",
    "NOARP",

    "PROMISC",
    "ALLMULTI",
    "MASTER",
    "SLAVE",

    "MC",
    "PORTSEL",
    "AUTOMEDIA",
    "DYNAMIC",

    "LINK_UP",
    "DORMANT",
    "UNDEFINED",
    "UNDEFINED",

    "UNDEFINED",
    "UNDEFINED",
    "UNDEFINED",
    "UNDEFINED",

    "UNDEFINED",
    "UNDEFINED",
    "UNDEFINED",
    "UNDEFINED",
    "UNDEFINED",
    "UNDEFINED",
    "UNDEFINED",
    "UNDEFINED",
};

static void if_flags_desc(int flags, char *desc) {
    desc[0] = '\0';
    for (int i = 18; i >= 0; i--) {
        if (flags & (1 << i)) {
            strcat(desc, " ");
            strcat(desc, flag_desc_tbl[i]);
        }
    }
}


/*
 * Decode a RTM_NEWADDR or RTM_DELADDR message.
 */
static int parseIfAddrMessage
(int type, struct ifaddrmsg *ifaddr, int rtasize, char *ifname,
pfunc_is_interested_event pfunc_check)
{
    struct rtattr *rta;
    struct ifa_cacheinfo *cacheinfo = NULL;
    char addrstr[INET6_ADDRSTRLEN] = "";

    // Sanity check.
    if (type != RTM_NEWADDR && type != RTM_DELADDR) {
        ALOGE("%s on incorrect message type 0x%x" , __FUNCTION__, type);
        return -1;
    }

    // For log messages.
    const char *msgtype = (type == RTM_NEWADDR) ? "RTM_NEWADDR" : "RTM_DELADDR";

    for (rta = IFA_RTA(ifaddr); RTA_OK(rta, rtasize);
         rta = RTA_NEXT(rta, rtasize)) {
        if (rta->rta_type == IFA_ADDRESS) {
            // Only look at the first address, because we only support notifying
            // one change at a time.
            if (*addrstr != '\0') {
                ALOGE("Multiple IFA_ADDRESSes in %s, ignoring" , msgtype);
                continue;
            }

            // Convert the IP address to a string.
            if (ifaddr->ifa_family == AF_INET) {
                struct in_addr *addr4 = (struct in_addr *) RTA_DATA(rta);
                if (RTA_PAYLOAD(rta) < sizeof(*addr4)) {
                    ALOGE("Short IPv4 address (%d bytes) in %s" ,
                          RTA_PAYLOAD(rta), msgtype);
                    continue;
                }
                inet_ntop(AF_INET, addr4, addrstr, sizeof(addrstr));
            } else if (ifaddr->ifa_family == AF_INET6) {
                struct in6_addr *addr6 = (struct in6_addr *) RTA_DATA(rta);
                if (RTA_PAYLOAD(rta) < sizeof(*addr6)) {
                    ALOGE("Short IPv6 address (%d bytes) in %s" ,
                          RTA_PAYLOAD(rta), msgtype);
                    continue;
                }
                inet_ntop(AF_INET6, addr6, addrstr, sizeof(addrstr));
            } else {
                ALOGE("Unknown address family %d" , ifaddr->ifa_family);
                continue;
            }

            // Find the interface name.
            if (!if_indextoname(ifaddr->ifa_index, ifname)) {
                ALOGE("Unknown ifindex %d in %s" , ifaddr->ifa_index, msgtype);
                return -1;
            }

            if (pfunc_check && !pfunc_check(ifname)) {
                ALOGI("pfunc_check error");
                return -1;
            }
            ALOGI("Address %s %s/%d %s %u %u" ,
                (type == RTM_NEWADDR) ? "updated": "removed",
                addrstr, ifaddr->ifa_prefixlen, ifname, ifaddr->ifa_flags, ifaddr->ifa_scope );
        } else if (rta->rta_type == IFA_CACHEINFO) {
            // Address lifetime information.
            if (cacheinfo) {
                // We only support one address.
                ALOGE("Multiple IFA_CACHEINFOs in %s, ignoring\n" , msgtype);
                continue;
            }

            if (RTA_PAYLOAD(rta) < sizeof(*cacheinfo)) {
                ALOGE("Short IFA_CACHEINFO (%d vs. %d bytes) in %s" ,
                      RTA_PAYLOAD(rta), sizeof(cacheinfo), msgtype);
                continue;
            }

            cacheinfo = (struct ifa_cacheinfo *) RTA_DATA(rta);
        }
    }

    if (addrstr[0] == '\0') {
        ALOGE("No IFA_ADDRESS in %s" , msgtype);
        return -1;
    }

    return 0;
}


static void parseBinaryNetlinkMessage
(char *buff, int len, char *rbuf, int rbufsize, int *pguard, pfunc_is_interested_event pfunc_check)
{
    char *result = NULL;
    unsigned int guard;
    int nAttrLen;
    struct nlmsghdr *nh;
    struct rtattr *pstruAttr;
    char *ifname = (char*)"UNKNOWN_NAME";

    guard = *pguard;
    result = rbuf + guard;

    rbuf[0] = '\0';
    for (nh = (struct nlmsghdr *) buff; NLMSG_OK (nh, (unsigned int)len);
         nh = NLMSG_NEXT (nh, len))
    {
        if (nh->nlmsg_type == NLMSG_DONE) {
            ALOGE("Did not find useful eth interface information");
            return;
        }

        if (nh->nlmsg_type == NLMSG_ERROR) {
            /* Do some error handling. */
            ALOGE("Read device name failed");
            return;
        }

        if (nh->nlmsg_type == RTM_DELLINK ||
            nh->nlmsg_type == RTM_NEWLINK) {
            struct ifinfomsg *einfo;
            int type = nh->nlmsg_type;
            char *desc;

            einfo = (struct ifinfomsg *)NLMSG_DATA(nh);

            pstruAttr = IFLA_RTA(einfo);
            nAttrLen = NLMSG_PAYLOAD(nh, sizeof(struct ifinfomsg));

            while (RTA_OK(pstruAttr, nAttrLen))
            {
                switch (pstruAttr->rta_type)
                {
                    case IFLA_IFNAME:
                        ifname = (char *)RTA_DATA(pstruAttr);
                        break;

                    default:
                        break;
                }

                pstruAttr = RTA_NEXT(pstruAttr, nAttrLen);
            }

            if (pfunc_check && !pfunc_check(ifname)) {
                continue;
            }

            if (type == RTM_NEWLINK &&
                (!(einfo->ifi_flags & IFF_LOWER_UP))) {
                    type = RTM_DELLINK;
            }

            desc = (char*)((type == RTM_DELLINK) ? "DELLINK" : "NEWLINK");
            if_flags_desc(einfo->ifi_flags, flags_desc);

            ALOGI("%s: %s(%d), flags=0X%X(%s)", ifname, desc, type, einfo->ifi_flags, flags_desc);

            snprintf(result,rbufsize-guard, "%s:%d:",ifname,type);
            guard += strlen(result);
            result += guard;
            *pguard = guard;
        }
        else if (nh->nlmsg_type == RTM_DELADDR || nh->nlmsg_type == RTM_NEWADDR) {
            int len = nh->nlmsg_len - sizeof(*nh);
            struct ifaddrmsg *ifa;
            char ifname[IFNAMSIZ + 1] = {0};

            if (sizeof(*ifa) > (size_t) len) {
                ALOGE("Got a short RTM_xxxADDR message\n");
                continue;
            }

            ifa = (struct ifaddrmsg *)NLMSG_DATA(nh);
            size_t rtasize = IFA_PAYLOAD(nh);
            if (0 != parseIfAddrMessage(nh->nlmsg_type, ifa, rtasize, ifname, pfunc_check)) {
                continue;
            }
            snprintf(result,rbufsize-guard, "%s:%d:",ifname,nh->nlmsg_type);
            guard += strlen(result);
            result += guard;
            *pguard = guard;
        }

    }
    rbuf[guard] = '\0';

    if (rbuf[0]) ALOGI("%s",rbuf);
}


/* If the string between 'str' and 'end' begins with 'prefixlen' characters
 * from the 'prefix' array, then return 'str + prefixlen', otherwise return
 * NULL.
 */
static const char*
has_prefix(const char* str, const char* end, const char* prefix, size_t prefixlen)
{
    if ((end-str) >= (ptrdiff_t)prefixlen && !memcmp(str, prefix, prefixlen))
        return str + prefixlen;
    else
        return NULL;
}


/*
 * Parse an ASCII-formatted message from a NETLINK_KOBJECT_UEVENT
 * netlink socket.
 */
static void parseAsciiNetlinkMessage
(char *buff, int len, char *rbuf, int rbufsize, int *pguard, pfunc_is_interested_event pfunc_check)
{
    int guard;
    char *result = NULL;
    unsigned int left;
    const char *s = buff;
    const char *end;
    int param_idx = 0;
    int i;
    int first = 1;
    const char *action = NULL;
    int seq;
    char *subsys = NULL;
    char *pch;
    char *net_if = NULL;

    if (len == 0)
        return;
    guard = *pguard;
    result = rbuf + guard;

    ALOGV(">>>>>>KOBJECT_UEVENT");
    for (pch = buff; pch < buff + len;) {
        ALOGV("%s", pch);
        pch = strchr(pch,'\0');
        pch++;
    }
    ALOGV("<<<<<<KOBJECT_UEVENT");

    /* Ensure the buff is zero-terminated, the code below depends on this */
    buff[len-1] = '\0';

    end = s + len;
    while (s < end) {
        if (first) {
            const char *p;
            /* buff is 0-terminated, no need to check p < end */
            for (p = s; *p != '@'; p++) {
                if (!*p) { /* no '@', should not happen */
                    return;
                }
            }
            first = 0;
        } else {
            const char* a;
            if ((a = HAS_CONST_PREFIX(s, end, "ACTION=")) != NULL) {
                if (!strcmp(a, "add"))
                    action = "added";
                else if (!strcmp(a, "remove"))
                    action = "removed";
                else if (!strcmp(a, "change"))
                    action = "changed";
            } else if ((a = HAS_CONST_PREFIX(s, end, "SEQNUM=")) != NULL) {
                seq = atoi(a);
            } else if ((a = HAS_CONST_PREFIX(s, end, "SUBSYSTEM=")) != NULL) {
                subsys = strdup(a);
            } else if ((a = HAS_CONST_PREFIX(s, end, "INTERFACE=")) != NULL) {
                net_if = strdup(a);
            }         }
        s += strlen(s) + 1;
    }

    if (subsys && 0 == strcmp(subsys, "net") && net_if && action) {
        if (!pfunc_check || pfunc_check(net_if)) {
        ALOGI("%s %s", net_if, action);
        snprintf(result, rbufsize-guard, "%s:%s:",net_if,action);
        guard += strlen(result);
        }
    }
    if (net_if != NULL)
        free(net_if);
    if (subsys != NULL)
        free(subsys);
    rbuf[guard] = '\0';
    *pguard = guard;
    if (rbuf[0]) ALOGI("%s",rbuf);
    return;
}


static int waitForNetInterfaceEvent
(int nl_socket_netlink_route, int nl_socket_kobj_uevent,
char *rbuf, int *pguard,
pfunc_update_interface_list pfunc,
pfunc_is_interested_event pfunc_check)
{
    char *buff;
    struct iovec iov;
    struct msghdr msg;
    int len;
    fd_set readable;
    int result = -1;
    int maxfd;
    int old_guard;

    *pguard = 0;
    buff = (char *)malloc(NL_POLL_MSG_SZ);
    if (!buff) {
        ALOGE("Allocate poll buffer failed");
        goto error;
    }

    iov.iov_base = buff;
    iov.iov_len = NL_POLL_MSG_SZ;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    maxfd = (nl_socket_netlink_route >= nl_socket_kobj_uevent) ?
        nl_socket_netlink_route: nl_socket_kobj_uevent;

    FD_ZERO(&readable);
    FD_SET(nl_socket_netlink_route, &readable);
    FD_SET(nl_socket_kobj_uevent, &readable);

    while (1) {
        result = select(maxfd+1, &readable, NULL, NULL, NULL);
        if (result >= 0 || errno != EINTR) {
            break;
        }
    }

    if (result < 0) {
        goto error;
    }
    if (result == 0) {
        /* Timed out */
        goto error;
    }

    if (FD_ISSET(nl_socket_netlink_route, &readable)) {
        old_guard = *pguard;
        if ((len = recvmsg(nl_socket_netlink_route, &msg, 0)) >= 0) {
            parseBinaryNetlinkMessage(buff, len, rbuf, RET_STR_SZ, pguard,pfunc_check);
        }

        if (old_guard != *pguard)
            ALOGE("netlink_route socket readable: [%s]", rbuf);
    }

    if (FD_ISSET(nl_socket_kobj_uevent, &readable)) {
        old_guard = *pguard;
        if ((len = recvmsg(nl_socket_kobj_uevent, &msg, 0)) >= 0) {
            parseAsciiNetlinkMessage(buff, len, rbuf, RET_STR_SZ, pguard,pfunc_check);
            pfunc();
        }
        if (old_guard != *pguard)
            ALOGE("kobject_event socket readable: [%s]", rbuf);
    }

    if (buff) {
        free(buff);
    }
    return 0;

error:
    if (buff)
        free(buff);
    return -1;
}


static int open_NETLINK_socket(int netlinkFamily, int groups)
{
    struct sockaddr_nl nl_addr;
    int ret = -1;
    int mysocket;

    memset(&nl_addr, 0, sizeof(struct sockaddr_nl));
    nl_addr.nl_family = AF_NETLINK;
    nl_addr.nl_pid = 0;
    nl_addr.nl_groups = groups;

    /*
     *Create connection to netlink socket
     */
    mysocket = socket(AF_NETLINK,SOCK_DGRAM,netlinkFamily);
    if (mysocket < 0) {
        ALOGE("Can not create netlink poll socket" );
        goto error;
    }

    errno = 0;
    if (bind(mysocket, (struct sockaddr *)(&nl_addr),
            sizeof(struct sockaddr_nl))) {
        ALOGE("Can not bind to netlink poll socket,%s",strerror(errno));

        goto error;
    }

    return mysocket;
error:
    ALOGE("%s failed" ,__FUNCTION__);
    if (mysocket > 0)
        close(mysocket);
    return ret;
}


static int find_empty_node(interface_info_t *node_table, int nr_nodes)
{
    int i;
    for ( i = 0; i < nr_nodes; i++) {
        if (node_table[i].if_index == -1)
            return i;
    }
    return -1;
}



static int add_interface_node_to_list(interface_info_t *node_table, int nr_nodes, int if_index, char *if_name)
{
    /*
     *Todo: Lock here!!!!
     */
    int i;

    i = find_empty_node(node_table, nr_nodes);
    if (i < 0)
        return -1;

    node_table[i].if_index = if_index;
    node_table[i].name = strdup(if_name);

    return 0;
}


static int getInterfaceName(interface_info_t *node_table, int nr_nodes,
    int index, char *ifname) {
    int i, j;
    interface_info_t *info;

    for ( i = 0, j = -1; i < nr_nodes; i++) {
        info = node_table + i;
        if (node_table[i].if_index != -1)
            j++;
        if (j == index) {
            ALOGI("Found :%s",info->name);
            strcpy(ifname, info->name);
            return 0;
        }
    }

    ifname[0] = '\0';
    return -1;
}

