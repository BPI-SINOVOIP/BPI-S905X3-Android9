/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#ifndef _SUB_CONTROL_H_
#define _SUB_CONTROL_H_

#ifdef  __cplusplus
extern "C" {
#endif

int subtitle_poll_sub_fd(int sub_fd, int timeout);
int subtitle_get_sub_size_fd(int sub_fd);
int subtitle_read_sub_data_fd(int sub_fd, char *buf, int length);
int update_read_pointer(int sub_handle, int flag);

#ifdef  __cplusplus
}
#endif

#endif
