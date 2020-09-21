/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <linux/genetlink.h>
#include "ssv_smartlink.h"
#define SSV_SMARTLINK_VERSION "v1.0"
#define GLOBAL_NL_ID 999
enum {
    KSMARTLINK_ATTR_UNSPEC,
    KSMARTLINK_ATTR_ENABLE,
    KSMARTLINK_ATTR_SUCCESS,
    KSMARTLINK_ATTR_CHANNEL,
    KSMARTLINK_ATTR_PROMISC,
    KSMARTLINK_ATTR_RXFRAME,
    KSMARTLINK_ATTR_SI_CMD,
    KSMARTLINK_ATTR_SI_STATUS,
    KSMARTLINK_ATTR_SI_SSID,
    KSMARTLINK_ATTR_SI_PASS,
    __KSMARTLINK_ATTR_MAX,
};
#define KSMARTLINK_ATTR_MAX (__KSMARTLINK_ATTR_MAX - 1)
enum {
    KSMARTLINK_CMD_UNSPEC,
    KSMARTLINK_CMD_SMARTLINK,
    KSMARTLINK_CMD_SET_CHANNEL,
    KSMARTLINK_CMD_GET_CHANNEL,
    KSMARTLINK_CMD_SET_PROMISC,
    KSMARTLINK_CMD_GET_PROMISC,
    KSMARTLINK_CMD_RX_FRAME,
    KSMARTLINK_CMD_SMARTICOMM,
    KSMARTLINK_CMD_SET_SI_CMD,
    KSMARTLINK_CMD_GET_SI_STATUS,
    KSMARTLINK_CMD_GET_SI_SSID,
    KSMARTLINK_CMD_GET_SI_PASS,
    __KSMARTLINK_CMD_MAX,
};
#define KSMARTLINK_CMD_MAX (__KSMARTLINK_CMD_MAX - 1)
struct netlink_msg {
    struct nlmsghdr n;
    struct genlmsghdr g;
    char buf[MAX_PAYLOAD];
};
static int gnl_fd=-1;
static int gnl_id=GLOBAL_NL_ID;
#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na) ((void *)((char*)(na) + NLA_HDRLEN))
static int _ssv_trigger_smartlink(unsigned int enable)
{
    struct netlink_msg msg;
    struct nlattr *na;
    int mlength, retval;
    struct sockaddr_nl nladdr;
    msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    msg.n.nlmsg_type = gnl_id;;
    msg.n.nlmsg_flags = NLM_F_REQUEST;
    msg.n.nlmsg_seq = 0;
    msg.n.nlmsg_pid = getpid();
#ifdef SSV_SMARTLINK_DEBUG
    printf("KSMARTLINK_CMD_SMARTLINK\n");
#endif
    msg.g.cmd = KSMARTLINK_CMD_SMARTLINK;
    na = (struct nlattr *) GENLMSG_DATA(&msg);
    na->nla_type = KSMARTLINK_ATTR_ENABLE;
    mlength = sizeof(unsigned int);
    na->nla_len = mlength+NLA_HDRLEN;
    memcpy(NLA_DATA(na), &enable, mlength);
    msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    retval = sendto(gnl_fd, (char *)&msg, msg.n.nlmsg_len, 0,
                    (struct sockaddr *) &nladdr, sizeof(nladdr));
    if (retval < 0)
    {
        printf("Fail to send message to kernel\n");
        return retval;
    }
    return retval;
}
static int _ssv_set_channel(unsigned int ch)
{
    struct netlink_msg msg;
    struct nlattr *na;
    int mlength, retval;
    struct sockaddr_nl nladdr;
    msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    msg.n.nlmsg_type = gnl_id;;
    msg.n.nlmsg_flags = NLM_F_REQUEST;
    msg.n.nlmsg_seq = 0;
    msg.n.nlmsg_pid = getpid();
#ifdef SSV_SMARTLINK_DEBUG
    printf("KSMARTLINK_CMD_SET_CHANNEL channel[%d]\n",ch);
#endif
    msg.g.cmd = KSMARTLINK_CMD_SET_CHANNEL;
    na = (struct nlattr *) GENLMSG_DATA(&msg);
    na->nla_type = KSMARTLINK_ATTR_CHANNEL;
    mlength = sizeof(unsigned int);
    na->nla_len = mlength+NLA_HDRLEN;
    memcpy(NLA_DATA(na), &ch, mlength);
    msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    retval = sendto(gnl_fd, (char *)&msg, msg.n.nlmsg_len, 0,
                    (struct sockaddr *) &nladdr, sizeof(nladdr));
    if (retval < 0)
    {
        printf("Fail to send message to kernel\n");
        return retval;
    }
    return retval;
}
int ssv_smartlink_set_channel(unsigned int ch)
{
    if (gnl_fd < 0)
    {
        return SSV_ERR_START_SMARTLINK;
    }
    else
    {
        return _ssv_set_channel(ch);
    }
}
static int _ssv_get_channel(unsigned int *pch)
{
    struct netlink_msg msg;
    struct nlattr *na;
    int mlength, retval;
    struct sockaddr_nl nladdr;
    int len;
    msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    msg.n.nlmsg_type = gnl_id;;
    msg.n.nlmsg_flags = NLM_F_REQUEST;
    msg.n.nlmsg_seq = 0;
    msg.n.nlmsg_pid = getpid();
#ifdef SSV_SMARTLINK_DEBUG
    printf("KSMARTLINK_CMD_GET_CHANNEL\n");
#endif
    msg.g.cmd = KSMARTLINK_CMD_GET_CHANNEL;
    na = (struct nlattr *) GENLMSG_DATA(&msg);
    na->nla_type = KSMARTLINK_ATTR_UNSPEC;
    mlength = 0;
    na->nla_len = mlength+NLA_HDRLEN;
    msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    retval = sendto(gnl_fd, (char *)&msg, msg.n.nlmsg_len, 0,
                    (struct sockaddr *) &nladdr, sizeof(nladdr));
    if (retval < 0)
    {
        printf("Fail to send message to kernel\n");
        return retval;
    }
    len = recv(gnl_fd, &msg, sizeof(msg), 0);
    if (len > 0)
    {
        if (msg.n.nlmsg_type == NLMSG_ERROR)
        {
            printf("Error, receive NACK\n");
            return -1;
        }
        if (!NLMSG_OK((&msg.n), len))
        {
             printf("Invalid reply message received via Netlink\n");
             return -1;
        }
#ifdef SSV_SMARTLINK_DEBUG
        printf("Receive CMD GET CHANNEL\n");
#endif
        na = (struct nlattr *) GENLMSG_DATA(&msg);
        if (na->nla_type != KSMARTLINK_ATTR_CHANNEL)
        {
            printf("%s Receive nla_type ERROR\n",__FUNCTION__);
            return -1;
        }
        *pch = *(unsigned int *)NLA_DATA(na);
    }
    return -1;
}
int ssv_smartlink_get_channel(unsigned int *pch)
{
    if (gnl_fd < 0)
    {
        return SSV_ERR_START_SMARTLINK;
    }
    else
    {
        return _ssv_get_channel(pch);
    }
}
static int _ssv_set_promisc(unsigned int promisc)
{
    struct netlink_msg msg;
    struct nlattr *na;
    int mlength, retval;
    struct sockaddr_nl nladdr;
    msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    msg.n.nlmsg_type = gnl_id;;
    msg.n.nlmsg_flags = NLM_F_REQUEST;
    msg.n.nlmsg_seq = 0;
    msg.n.nlmsg_pid = getpid();
#ifdef SSV_SMARTLINK_DEBUG
    printf("KSMARTLINK_CMD_SET_PROMISC\n");
#endif
    msg.g.cmd = KSMARTLINK_CMD_SET_PROMISC;
    na = (struct nlattr *) GENLMSG_DATA(&msg);
    na->nla_type = KSMARTLINK_ATTR_PROMISC;
    mlength = sizeof(unsigned int);
    na->nla_len = mlength+NLA_HDRLEN;
    memcpy(NLA_DATA(na), &promisc, mlength);
    msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    retval = sendto(gnl_fd, (char *)&msg, msg.n.nlmsg_len, 0,
                    (struct sockaddr *) &nladdr, sizeof(nladdr));
    if (retval < 0)
    {
        printf("Fail to send message to kernel\n");
        return retval;
    }
    return retval;
}
int ssv_smartlink_set_promisc(unsigned int promisc)
{
    if (gnl_fd < 0)
    {
        return SSV_ERR_START_SMARTLINK;
    }
    else
    {
        return _ssv_set_promisc(promisc);
    }
}
static int _ssv_get_promisc(int sock_fd, unsigned int *promisc)
{
    struct netlink_msg msg;
    struct nlattr *na;
    int mlength, retval;
    struct sockaddr_nl nladdr;
    int len;
    msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    msg.n.nlmsg_type = gnl_id;;
    msg.n.nlmsg_flags = NLM_F_REQUEST;
    msg.n.nlmsg_seq = 0;
    msg.n.nlmsg_pid = getpid();
#ifdef SSV_SMARTLINK_DEBUG
    printf("KSMARTLINK_CMD_GET_PROMISC\n");
#endif
    msg.g.cmd = KSMARTLINK_CMD_GET_PROMISC;
    na = (struct nlattr *) GENLMSG_DATA(&msg);
    na->nla_type = KSMARTLINK_ATTR_UNSPEC;
    mlength = 0;
    na->nla_len = mlength+NLA_HDRLEN;
    msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    retval = sendto(gnl_fd, (char *)&msg, msg.n.nlmsg_len, 0,
                    (struct sockaddr *) &nladdr, sizeof(nladdr));
    if (retval < 0)
    {
        printf("Fail to send message to kernel\n");
        return retval;
    }
    len = recv(gnl_fd, &msg, sizeof(msg), 0);
    if (len > 0)
    {
        if (msg.n.nlmsg_type == NLMSG_ERROR)
        {
            printf("Error, receive NACK\n");
            return -1;
        }
        if (!NLMSG_OK((&msg.n), len))
        {
             printf("Invalid reply message received via Netlink\n");
             return -1;
        }
#ifdef SSV_SMARTLINK_DEBUG
        printf("Receive CMD GET PROMISC\n");
#endif
        na = (struct nlattr *) GENLMSG_DATA(&msg);
        if (na->nla_type != KSMARTLINK_ATTR_PROMISC)
        {
            printf("%s Receive nla_type ERROR\n",__FUNCTION__);
            return -1;
        }
        *promisc = *(unsigned int *)NLA_DATA(na);
    }
    return -1;
}
int ssv_smartlink_get_promisc(unsigned int *paccept)
{
    if (gnl_fd < 0)
    {
        return SSV_ERR_START_SMARTLINK;
    }
    else
    {
        return _ssv_get_promisc(gnl_fd, paccept);
    }
}
static int _ssv_set_si_cmd(int sock_fd, unsigned int command)
{
    struct netlink_msg msg;
    struct nlattr *na;
    int mlength, retval;
    struct sockaddr_nl nladdr;
    msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    msg.n.nlmsg_type = gnl_id;;
    msg.n.nlmsg_flags = NLM_F_REQUEST;
    msg.n.nlmsg_seq = 0;
    msg.n.nlmsg_pid = getpid();
#ifdef SSV_SMARTLINK_DEBUG
    printf("KSMARTLINK_CMD_SET_SI_CMD\n");
#endif
    msg.g.cmd = KSMARTLINK_CMD_SET_SI_CMD;
    na = (struct nlattr *) GENLMSG_DATA(&msg);
    na->nla_type = KSMARTLINK_ATTR_SI_CMD;
    mlength = sizeof(unsigned int);
    na->nla_len = mlength+NLA_HDRLEN;
    memcpy(NLA_DATA(na), &command, mlength);
    msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    retval = sendto(gnl_fd, (char *)&msg, msg.n.nlmsg_len, 0,
                    (struct sockaddr *) &nladdr, sizeof(nladdr));
    if (retval < 0)
    {
        printf("Fail to send message to kernel\n");
        return retval;
    }
     return retval;
}
int smarticomm_set_si_cmd(unsigned int command)
{
    if (gnl_fd < 0)
    {
        return SSV_ERR_SET_SI_CMD;
    }
    else
    {
        return _ssv_set_si_cmd(gnl_fd, command);
    }
}
static int _ssv_get_si_status(uint8_t *status)
{
    struct netlink_msg msg;
    struct nlattr *na;
    int mlength, retval;
    struct sockaddr_nl nladdr;
    int len;
    msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    msg.n.nlmsg_type = gnl_id;;
    msg.n.nlmsg_flags = NLM_F_REQUEST;
    msg.n.nlmsg_seq = 0;
    msg.n.nlmsg_pid = getpid();
#ifdef SSV_SMARTLINK_DEBUG
    printf("KSMARTLINK_CMD_GET_SI_STATUS\n");
#endif
    msg.g.cmd = KSMARTLINK_CMD_GET_SI_STATUS;
    na = (struct nlattr *) GENLMSG_DATA(&msg);
    na->nla_type = KSMARTLINK_ATTR_UNSPEC;
    mlength = 0;
    na->nla_len = mlength+NLA_HDRLEN;
    msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    retval = sendto(gnl_fd, (char *)&msg, msg.n.nlmsg_len, 0,
                    (struct sockaddr *) &nladdr, sizeof(nladdr));
    if (retval < 0)
    {
        printf("Fail to send message to kernel\n");
        return retval;
    }
    memset((void *)&msg,0x00,sizeof(struct netlink_msg));
    len = recv(gnl_fd, &msg, sizeof(msg), 0);
    if (len > 0)
    {
        if (msg.n.nlmsg_type == NLMSG_ERROR)
        {
            printf("Error, receive NACK\n");
            return -1;
        }
        if (!NLMSG_OK((&msg.n), len))
        {
             printf("Invalid reply message received via Netlink\n");
             return -1;
        }
#ifdef SSV_SMARTLINK_DEBUG
        printf("Receive CMD SET SI_STATUS\n");
#endif
        na = (struct nlattr *) GENLMSG_DATA(&msg);
        if (na->nla_type != KSMARTLINK_ATTR_SI_STATUS)
        {
            printf("%s Receive nla_type ERROR\n",__FUNCTION__);
            return -1;
        }
        strcpy(status,(char *)NLA_DATA(na));
    }
    return -1;
}
int smarticomm_get_si_status(uint8_t *status)
{
    if (gnl_fd < 0)
    {
        return SSV_ERR_GET_SI_STATUS;
    }
    else
    {
        return _ssv_get_si_status(status);
    }
}
static int _ssv_get_si_ssid(uint8_t *ssid)
{
    struct netlink_msg msg;
    struct nlattr *na;
    int mlength, retval;
    struct sockaddr_nl nladdr;
    int len;
    msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    msg.n.nlmsg_type = gnl_id;;
    msg.n.nlmsg_flags = NLM_F_REQUEST;
    msg.n.nlmsg_seq = 0;
    msg.n.nlmsg_pid = getpid();
#ifdef SSV_SMARTLINK_DEBUG
    printf("KSMARTLINK_CMD_GET_SI_SSID\n");
#endif
    msg.g.cmd = KSMARTLINK_CMD_GET_SI_SSID;
    na = (struct nlattr *) GENLMSG_DATA(&msg);
    na->nla_type = KSMARTLINK_ATTR_UNSPEC;
    mlength = 0;
    na->nla_len = mlength+NLA_HDRLEN;
    msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    retval = sendto(gnl_fd, (char *)&msg, msg.n.nlmsg_len, 0,
                    (struct sockaddr *) &nladdr, sizeof(nladdr));
    if (retval < 0)
    {
        printf("Fail to send message to kernel\n");
        return retval;
    }
    len = recv(gnl_fd, &msg, sizeof(msg), 0);
    if (len > 0)
    {
        if (msg.n.nlmsg_type == NLMSG_ERROR)
        {
            printf("Error, receive NACK\n");
            return -1;
        }
        if (!NLMSG_OK((&msg.n), len))
        {
             printf("Invalid reply message received via Netlink\n");
             return -1;
        }
#ifdef SSV_SMARTLINK_DEBUG
        printf("Receive CMD SET SI_SSID\n");
#endif
        na = (struct nlattr *) GENLMSG_DATA(&msg);
        if (na->nla_type != KSMARTLINK_ATTR_SI_SSID)
        {
            printf("%s Receive nla_type ERROR\n",__FUNCTION__);
            return -1;
        }
        strcpy(ssid,(char *)NLA_DATA(na));
    }
    return -1;
}
int smarticomm_get_si_ssid(uint8_t *ssid)
{
    if (gnl_fd < 0)
    {
        return SSV_ERR_GET_SI_SSID;
    }
    else
    {
        return _ssv_get_si_ssid(ssid);
    }
}
static int _ssv_get_si_pass(uint8_t *pass)
{
    struct netlink_msg msg;
    struct nlattr *na;
    int mlength, retval;
    struct sockaddr_nl nladdr;
    int len;
    msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    msg.n.nlmsg_type = gnl_id;;
    msg.n.nlmsg_flags = NLM_F_REQUEST;
    msg.n.nlmsg_seq = 0;
    msg.n.nlmsg_pid = getpid();
#ifdef SSV_SMARTLINK_DEBUG
    printf("KSMARTLINK_CMD_SI_PASS\n");
#endif
    msg.g.cmd = KSMARTLINK_CMD_GET_SI_PASS;
    na = (struct nlattr *) GENLMSG_DATA(&msg);
    na->nla_type = KSMARTLINK_ATTR_UNSPEC;
    mlength = 0;
    na->nla_len = mlength+NLA_HDRLEN;
    msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    retval = sendto(gnl_fd, (char *)&msg, msg.n.nlmsg_len, 0,
                    (struct sockaddr *) &nladdr, sizeof(nladdr));
    if (retval < 0)
    {
        printf("Fail to send message to kernel\n");
        return retval;
    }
    len = recv(gnl_fd, &msg, sizeof(msg), 0);
    if (len > 0)
    {
        if (msg.n.nlmsg_type == NLMSG_ERROR)
        {
            printf("Error, receive NACK\n");
            return -1;
        }
        if (!NLMSG_OK((&msg.n), len))
        {
             printf("Invalid reply message received via Netlink\n");
             return -1;
        }
#ifdef SSV_SMARTLINK_DEBUG
        printf("Receive CMD SET SI_PASS\n");
#endif
        na = (struct nlattr *) GENLMSG_DATA(&msg);
        if (na->nla_type != KSMARTLINK_ATTR_SI_PASS)
        {
            printf("%s Receive nla_type ERROR\n",__FUNCTION__);
            return -1;
        }
        strcpy(pass,(char *)NLA_DATA(na));
    }
    return -1;
}
int smarticomm_get_si_pass(uint8_t *pass)
{
    if (gnl_fd < 0)
    {
        return SSV_ERR_GET_SI_PASS;
    }
    else
    {
        return _ssv_get_si_pass(pass);
    }
}
static int _ssv_netlink_init(void)
{
    int fd;
    struct sockaddr_nl local;
    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (fd < 0)
    {
        printf("fail to create netlink socket\n");
        return -1;
    }
    memset(&local, 0, sizeof(local));
    local.nl_family = AF_NETLINK;
    local.nl_groups = 0;
    if (bind(fd, (struct sockaddr *) &local, sizeof(local)) < 0)
        goto error;
    return fd;
error:
    close(fd);
    return -1;
}
static int _ssv_netlink_close(int *psock_fd)
{
    if (psock_fd)
    {
        if (*psock_fd > 0)
        {
            close(*psock_fd);
        }
        *psock_fd = -1;
    }
    return 0;
}
int ssv_smartlink_start(void)
{
    int ret=-1;
    unsigned int ch=0;
    unsigned int accept=0;
    gnl_fd = _ssv_netlink_init();
    if (gnl_fd < 0)
    {
        ret = gnl_fd;
        goto out;
    }
    ret = _ssv_trigger_smartlink(1);
    if (ret < 0)
    {
        goto out;
    }
    ret = 0;
out:
    return ret;
}
int ssv_smartlink_stop(void)
{
    (void)_ssv_trigger_smartlink(0);
    (void)_ssv_netlink_close(&gnl_fd);
    return 0;
}
void hexdump(unsigned char *buf, int len)
{
    int i;
    printf("\n-----------------------------\n");
    printf("hexdump(len=%d):\n", len);
    for (i = 0; i < len; i++)
    {
        printf(" %02x", buf[i]);
        if ((i+1)%40 == 0)
            printf("\n");
    }
    printf("\n-----------------------------\n");
}
int ssv_smartlink_recv_packet(uint8_t *pOutBuf, unsigned int *pOutBufLen)
{
    int len;
    struct netlink_msg msg;
    struct nlattr *na;
    struct ssv_wireless_register *reg;
    len = recv(gnl_fd, &msg, sizeof(msg), 0);
    if (len > 0)
    {
        if (msg.n.nlmsg_type == NLMSG_ERROR)
        {
            printf("Error, receive NACK\n");
            return -1;
        }
        if (!NLMSG_OK((&msg.n), len))
        {
             printf("Invalid reply message received via Netlink\n");
             return -1;
        }
        na = (struct nlattr *) GENLMSG_DATA(&msg);
        if (na->nla_type != KSMARTLINK_ATTR_RXFRAME)
        {
            printf("%s Receive nla_type ERROR\n",__FUNCTION__);
            return -1;
        }
        *pOutBufLen = na->nla_len - NLA_HDRLEN;
        memcpy(pOutBuf,(unsigned char *)NLA_DATA(na),*pOutBufLen);
#ifdef SSV_SMARTLINK_DEBUG
        printf("Receive RX FRAME pOutBufLen[%d] nla_len[%d]\n",*pOutBufLen,na->nla_len);
#endif
        return 1;
    }
    return -1;
}
static int _ssv_trigger_smarticomm(unsigned int enable)
{
    struct netlink_msg msg;
    struct nlattr *na;
    int mlength, retval;
    struct sockaddr_nl nladdr;
    msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    msg.n.nlmsg_type = gnl_id;;
    msg.n.nlmsg_flags = NLM_F_REQUEST;
    msg.n.nlmsg_seq = 0;
    msg.n.nlmsg_pid = getpid();
#ifdef SSV_SMARTLINK_DEBUG
    printf("KSMARTLINK_CMD_SMARTICOMM\n");
#endif
    msg.g.cmd = KSMARTLINK_CMD_SMARTICOMM;
    na = (struct nlattr *) GENLMSG_DATA(&msg);
    na->nla_type = KSMARTLINK_ATTR_ENABLE;
    mlength = (sizeof(unsigned int));
    na->nla_len = mlength+NLA_HDRLEN;
    memcpy(NLA_DATA(na), &enable, mlength);
    msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    retval = sendto(gnl_fd, (char *)&msg, msg.n.nlmsg_len, 0,
                    (struct sockaddr *) &nladdr, sizeof(nladdr));
    if (retval < 0)
        printf("Fail to send message to kernel\n");
}
int smaricomm_start(void)
{
    int ret=-1;
    unsigned int ch=0;
    unsigned int accept=0;
    gnl_fd = _ssv_netlink_init();
    if (gnl_fd < 0)
    {
        ret = gnl_fd;
        goto out;
    }
    ret = _ssv_trigger_smarticomm(START_SMART_ICOMM);
    if (ret < 0)
    {
        goto out;
    }
    ret = 0;
out:
    return ret;
}
int smarticomm_stop(void)
{
    (void)_ssv_trigger_smarticomm(STOP_SMART_ICOMM);
    (void)_ssv_netlink_close(&gnl_fd);
    return 0;
}
static char version[64]="SSV SmartLink Verison: " SSV_SMARTLINK_VERSION;
char *ssv_smartlink_version(void)
{
    return version;
}
