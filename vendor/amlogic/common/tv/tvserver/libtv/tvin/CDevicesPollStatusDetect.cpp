/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CDevicesDetect"

#include "CTvin.h"
#include <CTvLog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>

#include <tvutils.h>
#include <tvconfig.h>

#include "CDevicesPollStatusDetect.h"

CDevicesPollStatusDetect::CDevicesPollStatusDetect()
{
    m_event.events = 0;
    m_avin_status[0].status = AVIN_STATUS_IN;
    m_avin_status[0].channel = AVIN_CHANNEL1;
    m_avin_status[1].status = AVIN_STATUS_IN;
    m_avin_status[1].channel = AVIN_CHANNEL1;
    m_hdmi_status = 0;
    mpObserver = NULL;
    mVdinDetectFd = -1;
    if (mEpoll.create() < 0) {
        return;
    }
    //avin
    if (mAvinDetectFile.openFile(AVIN_DETECT_PATH) > 0) {
        m_event.data.fd = mAvinDetectFile.getFd();
        m_event.events = EPOLLIN | EPOLLET;
        mEpoll.add(mAvinDetectFile.getFd(), &m_event);
    }
    //HDMI
    if (mHdmiDetectFile.openFile(HDMI_DETECT_PATH) > 0) {
        m_event.data.fd = mHdmiDetectFile.getFd();
        m_event.events = EPOLLIN | EPOLLET;
        mEpoll.add(mHdmiDetectFile.getFd(), &m_event);
    }
}

CDevicesPollStatusDetect::~CDevicesPollStatusDetect()
{
}

int CDevicesPollStatusDetect::startDetect()
{
    //signal status
    int fd = CTvin::getInstance()->VDIN_GetVdinDeviceFd();
    if (fd > 0) {
        m_event.data.fd = fd;
        m_event.events = EPOLLIN | EPOLLET;
        mEpoll.add(fd, &m_event);

        mVdinDetectFd = fd;

        LOGD("startDetect get vdin device fd :%d", fd);
    }
    this->run("CDevicesPollStatusDetect");
    return 0;
}

const char* CDevicesPollStatusDetect::inputToName(tv_source_input_t srcInput) {
    const char* inputName[] = {
        /*"INVALID",*/
        "TV",
        "AV1",
        "AV2",
        "YPBPR1",
        "YPBPR2",
        "HDMI1",
        "HDMI2",
        "HDMI3",
        "HDMI4",
        "VGA,",
        "MPEG",
        "DTV",
        "SVIDEO",
        "IPTV",
        "DUMMY",
        "SPDIF",
        "ADTV",
        "MAX"
    };

    if (SOURCE_INVALID == srcInput)
        return "INVALID";
    else
        return inputName[srcInput];
}

int CDevicesPollStatusDetect::SourceInputMaptoChipHdmiPort(tv_source_input_t source_input)
{
    tvin_port_t source_port = TVIN_PORT_NULL;
    source_port = CTvin::getInstance()->Tvin_GetSourcePortBySourceInput(source_input);
    switch (source_port) {
    case TVIN_PORT_HDMI0:
        return HDMI_DETECT_STATUS_BIT_A;
        break;
    case TVIN_PORT_HDMI1:
        return HDMI_DETECT_STATUS_BIT_B;
        break;
    case TVIN_PORT_HDMI2:
        return HDMI_DETECT_STATUS_BIT_C;
        break;
    case TVIN_PORT_HDMI3:
        return HDMI_DETECT_STATUS_BIT_D;
        break;
    default:
        return HDMI_DETECT_STATUS_BIT_A;
        break;
    }
}

tv_source_input_t CDevicesPollStatusDetect::ChipHdmiPortMaptoSourceInput(int port)
{
    switch (port) {
    case HDMI_DETECT_STATUS_BIT_A:
        return CTvin::getInstance()->Tvin_PortToSourceInput(TVIN_PORT_HDMI0);
        break;
    case HDMI_DETECT_STATUS_BIT_B:
        return CTvin::getInstance()->Tvin_PortToSourceInput(TVIN_PORT_HDMI1);
        break;
    case HDMI_DETECT_STATUS_BIT_C:
        return CTvin::getInstance()->Tvin_PortToSourceInput(TVIN_PORT_HDMI2);
        break;
    case HDMI_DETECT_STATUS_BIT_D:
        return CTvin::getInstance()->Tvin_PortToSourceInput(TVIN_PORT_HDMI3);
        break;
    default:
        return CTvin::getInstance()->Tvin_PortToSourceInput(TVIN_PORT_HDMI0);
        break;
    }
}

int CDevicesPollStatusDetect::GetSourceConnectStatus(tv_source_input_t source_input)
{
    int PlugStatus = -1;
    int hdmi_status = 0;
    int source = -1;
    int HdmiDetectStatusBit = SourceInputMaptoChipHdmiPort((tv_source_input_t)source_input);
    switch (source_input) {
    case SOURCE_AV2:
    case SOURCE_AV1: {
        struct report_data_s status[2];
        mAvinDetectFile.readFile((void *)(&status), sizeof(struct report_data_s) * 2);
        for (int i = 0; i < 2; i++) {
            if (status[i].channel == AVIN_CHANNEL1) {
                source = SOURCE_AV1;
            } else if (status[i].channel == AVIN_CHANNEL2) {
                source = SOURCE_AV2;
            }

            if (source == source_input) {
                if (status[i].status == AVIN_STATUS_IN) {
                    PlugStatus = CC_SOURCE_PLUG_IN;
                } else {
                    PlugStatus = CC_SOURCE_PLUG_OUT;
                }
                break;
            }
            m_avin_status[i] = status[i];
        }//end for

        break;
    }
    case SOURCE_HDMI1:
    case SOURCE_HDMI2:
    case SOURCE_HDMI3:
    case SOURCE_HDMI4:{
        mHdmiDetectFile.readFile((void *)(&hdmi_status), sizeof(int));
        if ((hdmi_status & HdmiDetectStatusBit) == HdmiDetectStatusBit) {
            PlugStatus = CC_SOURCE_PLUG_IN;
        } else {
            PlugStatus = CC_SOURCE_PLUG_OUT;
        }
        m_hdmi_status = hdmi_status;
        break;
    }
    default:
        LOGD("GetSourceConnectStatus not support source :%s", inputToName(source_input));
        break;
    }

    LOGD("GetSourceConnectStatus source :%s, status:%s", inputToName(source_input), (CC_SOURCE_PLUG_IN == PlugStatus)?"plug in":"plug out");
    return PlugStatus;
}

bool CDevicesPollStatusDetect::threadLoop()
{
    if ( mpObserver == NULL ) {
        return false;
    }

    LOGD("detect thread start");

    prctl(PR_SET_NAME, (unsigned long)"CDevicesPollStatusDetect thread loop");
    //init status
    mHdmiDetectFile.readFile((void *)(&m_hdmi_status), sizeof(int));
    mAvinDetectFile.readFile((void *)(&m_avin_status), sizeof(struct report_data_s) * 2);
    LOGD("CDevicesPollStatusDetect Loop, get init hdmi = 0x%x  avin[0].status = %d, avin[1].status = %d", m_hdmi_status, m_avin_status[0].status, m_avin_status[1].status);

    while (!exitPending()) { //requietexit() or requietexitWait() not call
        int num = mEpoll.wait();
        for (int i = 0; i < num; ++i) {
            int fd = (mEpoll)[i].data.fd;
            /**
             * EPOLLIN event
             */
            if ((mEpoll)[i].events & EPOLLIN) {
                if (fd == mAvinDetectFile.getFd()) {//avin
                    struct report_data_s status[2];
                    mAvinDetectFile.readFile((void *)(&status), sizeof(struct report_data_s) * 2);
                    for (int i = 0; i < 2; i++) {
                        int source = -1, plug = -1;
                        if (/*status[i].channel == m_avin_status[i].channel &&*/ status[i].status != m_avin_status[i].status) {
                            //LOGD("status[i].status != m_avin_status[i].status");
                            if (status[i].status == AVIN_STATUS_IN) {
                                plug = CC_SOURCE_PLUG_IN;
                            } else {
                                plug = CC_SOURCE_PLUG_OUT;
                            }

                            if (status[i].channel == AVIN_CHANNEL1) {
                                source = SOURCE_AV1;
                            } else if (status[i].channel == AVIN_CHANNEL2) {
                                source = SOURCE_AV2;
                            }

                            LOGD("%s detected\n", inputToName((tv_source_input_t)source));
                            if (mpObserver != NULL) {
                                mpObserver->onSourceConnect(source, plug);
                            }
                        }//not equal
                        m_avin_status[i] = status[i];
                    }
                } else if (fd == mHdmiDetectFile.getFd()) { //hdmi
                    int hdmi_status = 0;
                    mHdmiDetectFile.readFile((void *)(&hdmi_status), sizeof(int));
                    LOGD("HDMI detected\n");
                    int source = -1, plug = -1;
                    if ((hdmi_status & HDMI_DETECT_STATUS_BIT_A) != (m_hdmi_status  & HDMI_DETECT_STATUS_BIT_A) ) {
                        if ((hdmi_status & HDMI_DETECT_STATUS_BIT_A) == HDMI_DETECT_STATUS_BIT_A) {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_A);
                            plug = CC_SOURCE_PLUG_IN;
                        } else {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_A);
                            plug = CC_SOURCE_PLUG_OUT;
                        }
                        mpObserver->onSourceConnect(source, plug);
                    }

                    if ((hdmi_status & HDMI_DETECT_STATUS_BIT_B) != (m_hdmi_status  & HDMI_DETECT_STATUS_BIT_B) ) {
                        if ((hdmi_status & HDMI_DETECT_STATUS_BIT_B) == HDMI_DETECT_STATUS_BIT_B) {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_B);
                            plug = CC_SOURCE_PLUG_IN;
                        } else {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_B);
                            plug = CC_SOURCE_PLUG_OUT;
                        }
                        mpObserver->onSourceConnect(source, plug);
                    }

                    if ((hdmi_status & HDMI_DETECT_STATUS_BIT_C) != (m_hdmi_status  & HDMI_DETECT_STATUS_BIT_C) ) {
                        if ((hdmi_status & HDMI_DETECT_STATUS_BIT_C) == HDMI_DETECT_STATUS_BIT_C) {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_C);
                            plug = CC_SOURCE_PLUG_IN;
                        } else {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_C);
                            plug = CC_SOURCE_PLUG_OUT;
                        }
                        mpObserver->onSourceConnect(source, plug);
                    }

                    if ((hdmi_status & HDMI_DETECT_STATUS_BIT_D) != (m_hdmi_status  & HDMI_DETECT_STATUS_BIT_D) ) {
                        if ((hdmi_status & HDMI_DETECT_STATUS_BIT_D) == HDMI_DETECT_STATUS_BIT_D) {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_D);
                            plug = CC_SOURCE_PLUG_IN;
                        } else {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_D);
                            plug = CC_SOURCE_PLUG_OUT;
                        }
                        mpObserver->onSourceConnect(source, plug);
                    }

                    m_hdmi_status = hdmi_status;
                }else if ( fd == mVdinDetectFd ) {
                    LOGD("VDIN detected\n");
                    if (mpObserver != NULL) {
                        mpObserver->onVdinSignalChange();
                    }
                }

                if ((mEpoll)[i].events & EPOLLOUT) {

                }
            }
        }
    }//exit

    LOGD("CDevicesPollStatusDetect, exiting...\n");
    //return true, run again, return false,not run.
    return false;
}
