/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: Header file
 */

#ifndef DROIDLOGIC_HDMICECBASE_H_
#define DROIDLOGIC_HDMICECBASE_H_

#include <log/log.h>
#include <hardware/hdmi_cec.h>
#include <utils/RefBase.h>
#include <LogControl.h>

namespace android {

#define LOG_UNIT_TAG "CEC"
#define DEFAULT_LOG_BUFFER_LEN 1152

extern int __unit_log_print(int prio, const char *tag, const char *cec_tag, const char *fmt, ...);

enum {
    OPEN_CEC_DEVICE = 1,
    CLOSE_CEC_DEVICE,//2
    GET_VERSION,//3
    GET_VENDOR_ID,//4
    GET_PHYSICAL_ADDRESS,//5
    SEND_MESSAGE,//6
    IS_CONNECT,//7
    SET_AUDIO_RETURN_CHANNEL,//8
    SET_OPTION,//9
    CLEAR_LOGICAL_ADDRESS,//10
    ADD_LOGICAL_ADDRESS,//11
    GET_PORT_INFO,//12

    CONNECT,//13
    NOTIFY_CALLBACK,//14
};

enum hdmi_cec_event_type {
    HDMI_EVENT_ADD_LOGICAL_ADDRESS = 0x04,
    HDMI_EVENT_RECEIVE_MESSAGE = 0x08,
};

typedef struct hdmi_cec_event {
    int eventType;
    union {
        cec_message_t cec;
        hotplug_event_t hotplug;
        int logicalAddress;
    };
} hdmi_cec_event_t;

class HdmiCecEventListener: virtual public RefBase {
public:
    virtual void onEventUpdate(const hdmi_cec_event_t* event) = 0;
};

class HdmiCecBase {
public:
    HdmiCecBase();
    virtual ~HdmiCecBase() {}

    virtual int openCecDevice() = 0;
    virtual int closeCecDevice() = 0;
    // Get CEC version from driver, if getting failed, return value is -1.
    virtual int getVersion(int* version) = 0;
    // Get vendor id used for vendor command, if getting failed, return value is -1.
    virtual int getVendorId(uint32_t* vendorId) = 0;
    // Get physical address of device, if getting failed, return value is -1.
    virtual int getPhysicalAddress(uint16_t* addr) = 0;
    // Send message to other device. Note that it runs in IO thread.
    virtual int sendMessage(const cec_message_t* message, bool isExtend) = 0;

    // Get Port information on all the HDMI ports.
    virtual void getPortInfos(hdmi_port_info_t* list[], int* total) = 0;
    // Add a logical address to device.
    virtual int addLogicalAddress(cec_logical_address_t address) = 0;
    // Clear all logical address registered to the device.
    virtual void clearLogicaladdress() = 0;
    // Set a flag and its value.
    virtual void setOption(int flag, int value) = 0;
    // Set audio return channel status.
    virtual void setAudioReturnChannel(int port, bool flag) = 0;
    // Whether to hdmi device is connected to the given port.
    virtual bool isConnected(int port) = 0;

    void printCecMsgBuf(const char *msg_buf, int len);
    void printCecEvent(const hdmi_cec_event_t *event);
    void printCecMessage(const cec_message_t* message);
    void printCecMessage(const cec_message_t* message, int result);
    const char* getEventType(int eventType);
    void setLogLevel(int level);
    int getLogLevel();

private:
    const char *getResult(int result);

    int mLogLevel;
};


};//namespace android

#endif /* DROIDLOGIC_HDMICECBASE_H_ */
