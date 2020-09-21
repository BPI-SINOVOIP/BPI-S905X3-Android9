/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

#ifndef NETWRAPPER_CTRL_H
#define NETWRAPPER_CTRL_H

#define REQUEST_BUF_LEN 512

extern struct selabel_handle *sehandle;

typedef int (*pf_request_handler)(char * request);


struct netwrapper_ctrl {
    int s;
    struct sockaddr_un local;
    struct sockaddr_un dest;
};


#ifdef __cplusplus
extern "C" {
#endif

struct netwrapper_ctrl * netwrapper_ctrl_open
(const char *client_path, const char *server_path);

void netwrapper_ctrl_close(struct netwrapper_ctrl *ctrl);

int netwrapper_ctrl_request(struct netwrapper_ctrl *ctrl, const char *cmd, size_t cmd_len);

int netwrapper_register_handler(const char *request, pf_request_handler handler);


int netwrapper_main(const char *server_path);

#ifdef __cplusplus
}
#endif


#endif
