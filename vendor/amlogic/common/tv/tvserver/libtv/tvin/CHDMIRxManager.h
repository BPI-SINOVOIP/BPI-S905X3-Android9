/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _C_HDMI_RX_MANAGER_H_
#define _C_HDMI_RX_MANAGER_H_


#include "CTvin.h"

#define HDMI_IOC_MAGIC 'H'
#define HDMI_IOC_HDCP_ON	_IO(HDMI_IOC_MAGIC, 0x01)
#define HDMI_IOC_HDCP_OFF	_IO(HDMI_IOC_MAGIC, 0x02)
#define HDMI_IOC_EDID_UPDATE	_IO(HDMI_IOC_MAGIC, 0x03)
#define HDMI_IOC_PC_MODE_ON	_IO(HDMI_IOC_MAGIC, 0x04)
#define HDMI_IOC_PC_MODE_OFF	_IO(HDMI_IOC_MAGIC, 0x05)
#define HDMI_IOC_HDCP22_AUTO	_IO(HDMI_IOC_MAGIC, 0x06)
#define HDMI_IOC_HDCP22_FORCE14 _IO(HDMI_IOC_MAGIC, 0x07)

#define HDMI_IOC_HDCP_GET_KSV _IOR(HDMI_IOC_MAGIC, 0x09, struct _hdcp_ksv)


#define CS_HDMIRX_DEV_PATH    "/dev/hdmirx0"
#define HDMI_FORCE_COLOR_RANGE "/sys/module/tvin_hdmirx/parameters/force_color_range"
#define HDMIRX0_DEBUG_PATH "/sys/class/hdmirx/hdmirx0/debug"
#define HDMI_CEC_PORT_SEQUENCE "/sys/class/cec/port_seq"
#define HDMI_CEC_PORT_MAP "/sys/module/tvin_hdmirx/parameters/port_map"


typedef struct _hdcp_ksv {
    int bksv0;
    int bksv1;
} _hdcp_ksv;

class CHDMIRxManager {
    public:
        CHDMIRxManager();
        ~CHDMIRxManager();
        int HdmiRxEdidUpdate();
        int HdmiRxHdcpVerSwitch(tv_hdmi_hdcp_version_t version);
        int HdmiRxHdcpOnOff(tv_hdmi_hdcpkey_enable_t flag);
        int GetHdmiHdcpKeyKsvInfo(struct _hdcp_ksv *msg);
        int CalHdmiPortCecPhysicAddr();
        int SetHdmiPortCecPhysicAddr();
};

#endif/*_C_HDMI_RX_MANAGER_H_*/
