/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */

#define LOG_NDEBUG 0
#define LOG_CEE_TAG "IHdmiCecCallback"

#include "IHdmiCecCallback.h"

namespace android {

class BpHdmiCecCallback: public BpInterface<IHdmiCecCallback> {
public:
    BpHdmiCecCallback(const sp<IBinder> &impl) :
        BpInterface<IHdmiCecCallback> (impl)
    {}

    virtual void notifyCallback(const hdmi_cec_event_t* event)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecCallback::getInterfaceDescriptor());
        data.writeInt32(event->eventType);
        if (((event->eventType & HDMI_EVENT_CEC_MESSAGE) != 0) || ((event->eventType
                & HDMI_EVENT_RECEIVE_MESSAGE) != 0)) {
            data.writeInt32(event->cec.initiator);
            data.writeInt32(event->cec.destination);
            data.writeInt32(event->cec.length);
            data.write(event->cec.body, event->cec.length);
        } else if ((event->eventType & HDMI_EVENT_HOT_PLUG) != 0) {
            data.writeInt32(event->hotplug.connected);
            data.writeInt32(event->hotplug.port_id);
        } else if ((event->eventType & HDMI_EVENT_ADD_LOGICAL_ADDRESS) != 0) {
            data.writeInt32(event->logicalAddress);
        }
        remote()->transact(NOTIFY_CALLBACK, data, &reply);
    }

};

IMPLEMENT_META_INTERFACE(HdmiCecCallback, "droidlogic.IHdmiCecCallback");

//----------------------------------------------------------------------------------------

status_t BnHdmiCecCallback::onTransact(uint32_t code, const Parcel &data, Parcel *reply,
        uint32_t flags) {
    switch (code) {
        case NOTIFY_CALLBACK: {
            CHECK_INTERFACE(IHdmiCecCallback, data, reply);
            hdmi_cec_event_t event;
            event.eventType = data.readInt32();
            if (((event.eventType & HDMI_EVENT_CEC_MESSAGE) != 0) || ((event.eventType
                    & HDMI_EVENT_RECEIVE_MESSAGE) != 0)) {
                event.cec.initiator = (cec_logical_address_t) data.readInt32();
                event.cec.destination = (cec_logical_address_t) data.readInt32();
                event.cec.length = data.readInt32();
                data.read(event.cec.body, event.cec.length);
            } else if ((event.eventType & HDMI_EVENT_HOT_PLUG) != 0) {
                event.hotplug.connected = data.readInt32();
                event.hotplug.port_id = data.readInt32();
            } else if ((event.eventType & HDMI_EVENT_ADD_LOGICAL_ADDRESS) != 0) {
                event.logicalAddress = data.readInt32();
            }
            notifyCallback(&event);
            return NO_ERROR;
        }
        default: {
            return BBinder::onTransact(code, data, reply, flags);
        }
    }
    return NO_ERROR;
}

};//namespace android
