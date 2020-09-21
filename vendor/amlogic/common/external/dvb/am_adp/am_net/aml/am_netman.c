#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 网络管理程序
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-29: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 0

#include <am_debug.h>
#include <am_misc.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>
 
static int dhcp = 0;

static int get_value(const char *cmd, const char *title, char *value, int len, int line)
{
	char scmd[300];
	FILE *fp;
	char *st;
	int i = 0;

	/*The array size of value is big enough*/
	
	snprintf(scmd, sizeof(scmd), "%s > /tmp/netman_output", cmd);
	system(scmd);

	fp = fopen("/tmp/netman_output", "rt");
	if (! fp)
	{
		AM_DEBUG(0, "Cannot open /tmp/netman_output");
		return -1;
	}

	/*读取指定行*/
	while (i < line)
	{
		i++;
		if (! fgets(scmd, sizeof(scmd), fp))
		{
			AM_DEBUG(1, "Cannot read line %d from /tmp/netman_output", i);
			fclose(fp);
			return -1;
		}
	}
	
	st = strstr(scmd, title);
	if (! st)
	{
		AM_DEBUG(0, "Cannot find string '%s' from /tmp/netman_output", title);
		fclose(fp);
		return -1;
	}
	
	strncpy(value, st, len);
	
	fclose(fp);
	
	return 0;
}

int main(int argc, char **argv)
{
	int fd, confd;
	char buf[256];
	char cmd[256];
	char para[256];
	struct sockaddr_un addr;
	socklen_t len;

	system("rm -rf /tmp/netman_socket");
	if (AM_LocalServer("/tmp/netman_socket", &fd) != AM_SUCCESS)
	{
		AM_DEBUG(1, "Cannot create /tmp/netman_socket");

		return -1;
	}

	while (1)
	{
		len = SUN_LEN(&addr);
		confd = accept(fd, (struct sockaddr*)&addr, &len);
		if (AM_LocalGetResp(confd, buf, sizeof(buf)) == AM_SUCCESS)
		{
			AM_DEBUG(1, "Get response: %s", buf);
			cmd[0] = 0;
			para[0] = 0;
			sscanf(buf, "%s %s", cmd, para);

			if (! strcmp(cmd, "getinfo"))
			{
				system("ifconfig eth0 > /tmp/ifconf_output");

				/*DHCP*/
				if (dhcp)
					strcpy(buf, "yes");
				else
					strcpy(buf, "no");
				
				/*IP*/
				cmd[0] = 0;
				get_value("grep 'inet addr' /tmp/ifconf_output", "inet addr:", cmd, sizeof(cmd), 1);
				strcat(buf, " ");
				if (cmd[0])
				{
					sscanf(cmd + strlen("inet addr:"), "%s", para);
					strcat(buf, para);
				}
				else
				{
					strcat(buf, "none");
				}

				/*MASK*/
				cmd[0] = 0;
				get_value("grep Mask /tmp/ifconf_output", "Mask:", cmd, sizeof(cmd), 1);
				strcat(buf, " ");
				if (cmd[0])
				{
					sscanf(cmd + strlen("Mask:"), "%s", para);
					strcat(buf, para);
				}
				else
				{
					strcat(buf, "none");
				}

				/*GW*/
				cmd[0] = 0;
				get_value("route -n | grep ^0.0.0.0", "0.0.0.0", cmd, sizeof(cmd), 1);
				strcat(buf, " ");
				if (cmd[0])
				{
					sscanf(cmd, "%*s %s", para);
					strcat(buf, para);
				}
				else
				{
					strcat(buf, "none");
				}

				/*DNS1*/
				cmd[0] = 0;
				get_value("grep ^nameserver /etc/resolv.conf", "nameserver", cmd, sizeof(cmd), 1);
				strcat(buf, " ");
				if (cmd[0])
				{
					sscanf(cmd, "%*s %s", para);
					strcat(buf, para);
				}
				else
				{
					strcat(buf, "none");
				}

				/*DNS2*/
				cmd[0] = 0;
				get_value("grep ^nameserver /etc/resolv.conf", "nameserver", cmd, sizeof(cmd), 2);
				strcat(buf, " ");
				if (cmd[0])
				{
					sscanf(cmd, "%*s %s", para);
					strcat(buf, para);
				}
				else
				{
					strcat(buf, "none");
				}

				/*DNS3*/
				cmd[0] = 0;
				get_value("grep ^nameserver /etc/resolv.conf", "nameserver", cmd, sizeof(cmd), 3);
				strcat(buf, " ");
				if (cmd[0])
				{
					sscanf(cmd, "%*s %s", para);
					strcat(buf, para);
				}
				else
				{
					strcat(buf, "none");
				}

				/*DNS4*/
				cmd[0] = 0;
				get_value("grep ^nameserver /etc/resolv.conf", "nameserver", cmd, sizeof(cmd), 4);
				strcat(buf, " ");
				if (cmd[0])
				{
					sscanf(cmd, "%*s %s", para);
					strcat(buf, para);
				}
				else
				{
					strcat(buf, "none");
				}

				AM_LocalSendCmd(confd, buf);
				
			}
			else if (! strcmp(cmd, "setdhcp"))
			{
				if (!strcmp(para, "on") && !dhcp)
				{
					dhcp = 1;
					AM_DEBUG(0, "Start DHCP...");
					system("udhcpc");
				}
				else if (!strcmp(para, "off") && dhcp)
				{
					dhcp = 0;
					AM_DEBUG(0, "Stop DHCP...");
					system("killall -9 udhcpc");
				}
			}
			else if (! strcmp(cmd, "gethw"))
			{
				cmd[0] = 0;
				get_value("ifconfig eth0 | grep HWaddr", "HWaddr ", cmd, sizeof(cmd), 1);
				AM_LocalSendCmd(confd, cmd + strlen("HWaddr "));
			}
			else if (! strcmp(cmd, "sethw") && para[0])
			{
				AM_DEBUG(0, "Set MAC addr to: %s", para);
				snprintf(cmd, sizeof(cmd), "ifconfig eth0 hw ether %s", para);
				system(cmd);
			}
			else if (! strcmp(cmd, "setip"))
			{
				AM_DEBUG(0, "Set IP addr to: %s", para);
				snprintf(cmd, sizeof(cmd), "ifconfig eth0 %s", para);
				system("ifconfig eth0 down");
				system(cmd);
				system("ifconfig eth0 up");
			}
			else if (! strcmp(cmd, "setmask"))
			{
				AM_DEBUG(0, "Set NetMask to: %s", para);
				snprintf(cmd, sizeof(cmd), "ifconfig eth0 netmask %s", para);
				system("ifconfig eth0 down");
				system(cmd);
				system("ifconfig eth0 up");
			}
			else if (! strcmp(cmd, "setgw"))
			{
				/*删除default gw*/
				/*GW*/
				cmd[0] = 0;
				get_value("route -n | grep ^0.0.0.0", "0.0.0.0", cmd, sizeof(cmd), 1);
				if (cmd[0])
				{
					sscanf(cmd, "%*s %s", buf);
					snprintf(cmd, sizeof(cmd), "route del default gw %s eth0", buf);
					system(cmd);
				}

				AM_DEBUG(0, "Set Gateway to: %s", para);
				snprintf(cmd, sizeof(cmd), "route add default gw %s eth0", para);
				system(cmd);
			}
			else if (! strcmp(cmd, "setdns1"))
			{
				AM_DEBUG(0, "Set DNS1 to: %s", para);
				system("echo domain AmlogicSTB > /etc/resolv.conf");
				snprintf(cmd, sizeof(cmd), "echo nameserver %s >> /etc/resolv.conf", para);
				system(cmd);
			}
			else if (! strcmp(cmd, "setdns2"))
			{
				AM_DEBUG(0, "Set DNS2 to: %s", para);
				snprintf(cmd, sizeof(cmd), "echo nameserver %s >> /etc/resolv.conf", para);
				system(cmd);
			}
			else if (! strcmp(cmd, "setdns3"))
			{
				AM_DEBUG(0, "Set DNS3 to: %s", para);
				snprintf(cmd, sizeof(cmd), "echo nameserver %s >> /etc/resolv.conf", para);
				system(cmd);
			}
			else if (! strcmp(cmd, "setdns4"))
			{
				AM_DEBUG(0, "Set DNS4 to: %s", para);
				snprintf(cmd, sizeof(cmd), "echo nameserver %s >> /etc/resolv.conf", para);
				system(cmd);
			}
			else
			{
				AM_DEBUG(1, "Unknown command %s", buf);
			}
		}
		close(confd);
	}
}

