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
 * \brief IP视频服务器
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-11: create the document
 ***************************************************************************/

#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef ANDROID
#include "sys/socket.h"
#endif

static void usage(void)
{
	printf("usage: am_av_server FILE PORT\n");
}

int main(int argc, char **argv)
{
	char *fname;
	int port = 1234;
	int sock, c_sock;
	struct sockaddr_in addr;
	struct sockaddr c_addr;
	socklen_t c_addr_len;
	
	if(argc<2)
	{
		usage();
		return 1;
	}
	
	fname = argv[1];
	
	if(argc>2)
		port = strtol(argv[2], NULL, 0);
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock==-1)
	{
		fprintf(stderr, "create socket failed\n");
		return 1;
	}
	
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	
	listen(sock, 5);
	
	printf("create the server\n");
	
	while(1)
	{
		int fd, left = 0, cnt;
		static char buf[64*1024];
		
		c_addr_len = sizeof(c_addr);
		c_sock = accept(sock, &c_addr, &c_addr_len);
		if(c_sock==-1)
			continue;
		
		printf("get a client\n");
		
		fd = open(fname, O_RDONLY);
		if(fd==-1)
		{
			fprintf(stderr, "cannot open \"%s\"\n", fname);
			goto close_c;
		}
		printf("open file \"%s\"\n", fname);
		
		while(1)
		{
			cnt = sizeof(buf)-left;
			cnt = read(fd, buf+left, cnt);
			if(cnt>0)
			{
				left += cnt;
				printf("read %d bytes\n", cnt);
			}
			else if(cnt==-1)
			{
				fprintf(stderr, "read file failed\n");
				break;
			}
			if(left)
			{
				cnt = write(c_sock, buf, left);
				if(cnt>0)
				{
					left -= cnt;
					if(left)
						memmove(buf, buf+cnt, left);
					printf("send %d bytes\n", cnt);
				}
				else if(cnt==-1)
				{
					fprintf(stderr, "send data failed\n");
					break;
				}
			}
		}

		printf("server break!\n");
		
		close(fd);
close_c:		
		close(c_sock);
	}
	
	close(sock);
	return 0;
}
 
