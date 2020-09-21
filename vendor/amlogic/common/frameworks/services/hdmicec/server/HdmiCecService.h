/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

#ifndef _HDMI_CEC_SERVICE_H_
#define _HDMI_CEC_SERVICE_H_

#include <utils/Vector.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/Mutex.h>
#include <HdmiCecControl.h>
#include <IHdmiCecService.h>


namespace android {
class HdmiCecService : public BnHdmiCecService, public HdmiCecEventListener {
public:
    HdmiCecService();
    virtual ~ HdmiCecService();

    virtual int openCecDevice();
    virtual int closeCecDevice();
    virtual int getVersion(int* version);
    virtual int getVendorId(uint32_t* vendorId);
    virtual int getPhysicalAddress(uint16_t* addr);
    virtual int sendMessage(const cec_message_t* message, bool isExtend);

    virtual void getPortInfos(hdmi_port_info_t* list[], int* total);
    virtual int addLogicalAddress(cec_logical_address_t address);
    virtual void clearLogicaladdress();
    virtual void setOption(int flag, int value);
    virtual void setAudioReturnChannel(int port, bool flag);
    virtual bool isConnected(int port);

    virtual int connect(const sp<IHdmiCecCallback> &client);
    virtual void onEventUpdate(const hdmi_cec_event_t* event);

    static void instantiate();

    virtual status_t dump(int fd, const Vector<String16>& args);
private:
    void dumpHelp(String8 &result);

    HdmiCecControl *mHdmiCecControl;
    Vector<sp<IHdmiCecCallback>> mClients;

    mutable Mutex mLock;
};

}; //namespace android
#endif /* _HDMI_CEC_SERVICE_H_ */
