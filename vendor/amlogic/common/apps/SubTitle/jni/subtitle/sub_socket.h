/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#ifndef SUB_SOCKET_H
#define SUB_SOCKET_H

#include <stdint.h>

#define TYPE_TOTAL          1
#define TYPE_STARTPTS       2
#define TYPE_SUBTYPE        3

#ifdef  __cplusplus
extern "C" {
#endif

#define QUEUE_SIZE          10
#define BUFFER_SIZE         1024
#define LOOP_BUFFER_SIZE    256*1024
#define LISTEN_PORT 10100

static int mSockFd;
static bool mStop;
static bool mClientStop;
static char *mLoopBuf;
static char *mRPtr;
static char *mWPtr;
static int mTotal;
static int mType;
static int64_t mTimeUs;
static int64_t mStartPts;
static int64_t mSize;

static char subInfoStr[2048] = {0};
static char subTypeStr[1024] = {0};
static char subLanStr[1024] = {0};

//client parameters
static int client_list[QUEUE_SIZE];
static int client_num;

static bool isStartServerTread;

static void* startServerThread(void* arg);
static void child_connect(int sockfd);
static void safeCopy(char* sPtr, char* src, int size);
void safeRead(char* sPtr, char* des, int size);
int getDataSize(char* sPtr);

void startServer();
void stopServer();
int getSizeBySkt();
void getDataBySkt(char *buf, int size);
int getInfoBySkt(int type);
void getInSubTypeStrBySkt(char* subTypeStr);
void getInSubLanStrBySkt(char* subLanStr);
void resetSocketBuffer();
void getPcrscrBySkt(char* pcrStr);

#ifdef  __cplusplus
}
#endif

#endif
