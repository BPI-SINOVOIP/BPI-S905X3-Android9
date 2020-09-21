/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef C_SOURCE_CONNECT_DETECT_H
#define C_SOURCE_CONNECT_DETECT_H

#include <utils/Thread.h>
#include <CFile.h>
#include <zepoll.h>

enum avin_status_e {
    AVIN_STATUS_IN = 0,
    AVIN_STATUS_OUT = 1,
    AVIN_STATUS_UNKNOW = 2,
};
enum avin_channel_e {
    AVIN_CHANNEL1 = 0,
    AVIN_CHANNEL2 = 1,
};

struct report_data_s {
    enum avin_channel_e channel;
    enum avin_status_e status;
};

/*
3	R	0	stat_5v_portD:
Status of 5v power for port D.
2	R	0	stat_5v_portC:
Status of 5v power for port C.
1	R	0	stat_5v_portB:
Status of 5v power for port B.
0	R	0	stat_5v_portA:
Status of 5v power for port A.
 */
static const int HDMI_DETECT_STATUS_BIT_A = 0x01;
static const int HDMI_DETECT_STATUS_BIT_B = 0x02;
static const int HDMI_DETECT_STATUS_BIT_C = 0x04;
static const int HDMI_DETECT_STATUS_BIT_D = 0x08;

static const char *AVIN_DETECT_PATH = "/dev/avin_detect";
static const char *HDMI_DETECT_PATH = "/dev/hdmirx0";
static const char *VPP_POLL_PATCH = "/dev/amvideo_poll";

using namespace android;
class CDevicesPollStatusDetect: public Thread {
public:
    CDevicesPollStatusDetect();
    ~CDevicesPollStatusDetect();
    int startDetect();
    int GetSourceConnectStatus(tv_source_input_t source_input);
    int SourceInputMaptoChipHdmiPort(tv_source_input_t source_input);
    tv_source_input_t ChipHdmiPortMaptoSourceInput(int port);

    class ISourceConnectObserver {
    public:
        ISourceConnectObserver() {};
        virtual ~ISourceConnectObserver() {};
        virtual void onSourceConnect(int source __unused, int connect_status __unused) {};
        virtual void onVdinSignalChange() {};
    };

    void setObserver ( ISourceConnectObserver *pOb ) {
        mpObserver = pOb;
    };
private:
    bool threadLoop();
    const char* inputToName(tv_source_input_t srcInput);

    int mVdinDetectFd;
    ISourceConnectObserver *mpObserver;
    Epoll       mEpoll;
    mutable Mutex mLock;
    epoll_event m_event;
    CFile mAvinDetectFile;
    CFile mHdmiDetectFile;
    CFile mVppPollFile;
    struct report_data_s m_avin_status[2];
    int m_hdmi_status;
};
#endif
