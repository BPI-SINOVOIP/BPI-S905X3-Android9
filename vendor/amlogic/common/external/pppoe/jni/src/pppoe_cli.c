/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <sys/types.h>
#include <errno.h>

#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/un.h>
#include "netwrapper.h"

#include "pppoe_status.h"

static char pppd_connect_cmd[512];
static char pppoe_plugin_cmd[] = {"'pppoe -p " _ROOT_PATH "/etc/ppp/pppoe.pid" " -I eth0 -T 80 -U -m 1412'" };
static char pppd_options[] = {"debug logfd 1 noipdefault noauth default-asyncmap defaultroute nodetach mtu 1492 mru 1492 noaccomp nodeflate nopcomp novj novjccomp lcp-echo-interval 20 lcp-echo-failure 3"};

static int usage()
{
    fprintf(stderr,"ppp_cli connect <user> <pwd>\n");
    fprintf(stderr,"ppp_cli disconnect\n");

    return -1;
}


int main(int argc, char *argv[])
{
    int i;
    struct netwrapper_ctrl * ctrl;
    
    if (argc < 2 || (0 != strcmp( "connect", argv[1]) &&
                     0 != strcmp( "disconnect", argv[1] ))) {
        usage();
        return -2;
    }

    ctrl = netwrapper_ctrl_open(_ROOT_PATH "/etc/ppp/pppcli", PPPOE_WRAPPER_SERVER_PATH);
	if (ctrl == NULL) {
    	printf("Failed to connect to pppd\n");
    	return -1;
    }

    if (0 == strcmp( "connect", argv[1])) {
        sprintf(pppd_connect_cmd, "pppd pty %s %s user %s password %s &", 
                pppoe_plugin_cmd, pppd_options, argv[2], argv[3] );    

    }
    else if (0 == strcmp( "disconnect", argv[1])) {
        sprintf(pppd_connect_cmd, "ppp-stop");    
    }


    netwrapper_ctrl_request(ctrl, pppd_connect_cmd, strlen(pppd_connect_cmd));

    netwrapper_ctrl_close(ctrl);
    exit(0);
}
