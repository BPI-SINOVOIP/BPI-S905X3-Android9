/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

#ifndef PPPOE_STATUS_H
#define PPPOE_STATUS_H

#define PPP_STATUS_CONNECTED  0x10
#define PPP_STATUS_DISCONNECTED  0x20
#define PPP_STATUS_CONNECTING  0x40

//#ifndef _ROOT_PATH
#define _ROOT_PATH "/data/misc"
#define PPPOE_PIDFILE _ROOT_PATH "/etc/ppp/pppoe.pid"
#define PPPOE_WRAPPER_CLIENT_PATH _ROOT_PATH "/etc/ppp/pppoe"
#define PPPOE_WRAPPER_SERVER_PATH "/dev/socket/pppoe_wrapper"

#ifdef __cplusplus
extern "C" {
#endif

int get_net_updown(const char *phy_if_name);
int get_pppoe_status(const char *phy_if_name);

#ifdef __cplusplus
}
#endif

#endif

