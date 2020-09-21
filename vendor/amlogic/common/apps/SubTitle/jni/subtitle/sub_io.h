/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#ifndef SUB_IO_H
#define SUB_IO_H

#define TYPE_TOTAL          1
#define TYPE_STARTPTS       2
#define TYPE_SUBTYPE        3

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
    IO_TYPE_DEV = 0,
    IO_TYPE_SOCKET = 1,
    IO_TYPE_END = 0xFF,
} IOType;

IOType mIOType;

//for dev
int pollFd(int sub_fd, int timeout);

//for socket
void startSocketServer();
void stopSocketServer();
int getInfo(int type);

// for common
int getSize(int sub_fd);
void getData(int sub_fd, char *buf, int size);
void setIOType(IOType type);
IOType getIOType();
void getInSubTypeStrs(char *subTypeStr);
void getInSubLanStrs(char *subLanStr);
void getPcrscr(char* pcrStr);

#ifdef  __cplusplus
}
#endif

#endif
