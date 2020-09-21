/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: Header file
 */

#ifndef DROIDLOGIC_HDMICECCLIENT_H_
#define DROIDLOGIC_HDMICECCLIENT_H_

#include <binder/IBinder.h>
#include <utils/threads.h>

#include "HdmiCecBase.h"
#include "IHdmiCecCallback.h"
#include "IHdmiCecService.h"

namespace android {

class HdmiCecClient: public HdmiCecBase, public BnHdmiCecCallback {
public:
    static sp<HdmiCecClient> connect();

    virtual int openCecDevice();
    virtual int closeCecDevice();
    virtual int getVersion(int* version);
    virtual int getVendorId(uint32_t* vendorId);
    virtual int getPhysicalAddress(uint16_t* addr);
    virtual int sendMessage(const cec_message_t* message, bool isExtend = false);

    virtual void getPortInfos(hdmi_port_info_t* list[], int* total);
    virtual int addLogicalAddress(cec_logical_address_t address);
    virtual void clearLogicaladdress();
    virtual void setOption(int flag, int value);
    virtual void setAudioReturnChannel(int port, bool flag);
    virtual bool isConnected(int port);

    virtual void notifyCallback(const hdmi_cec_event_t* event);
    void setEventObserver(const sp<HdmiCecEventListener> &eventListener);

private:
    HdmiCecClient();
    virtual ~HdmiCecClient();

    static Mutex mLock;
    static sp<IHdmiCecService> mHdmiCecService;
    sp<HdmiCecEventListener> mEventListener;

    static const sp<IHdmiCecService> getHdmiCecService();

    class DeathNotifier: public IBinder::DeathRecipient {
    public:
        DeathNotifier() {
        }
        virtual void binderDied(const wp<IBinder>& who);
    };
    static sp<DeathNotifier> mDeathNotifier;
};

}
;//namespace android

#endif /* DROIDLOGIC_HDMICECCLIENT_H_ */
